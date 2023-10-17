/*
    SPDX-FileCopyrightText: 2021 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ImagePlatformKWin.h"
#include "ExportManager.h"

#include <KWindowSystem>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>
#include <QFile>
#include <QFileDevice>
#include <QFuture>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QPixmap>
#include <QScreen>
#include <QTimer>
#include <QtConcurrentRun>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

using namespace Qt::StringLiterals;

static QVariantMap screenShotFlagsToVardict(ImagePlatformKWin::ScreenShotFlags flags)
{
    QVariantMap options;

    if (flags & ImagePlatformKWin::ScreenShotFlag::IncludeCursor) {
        options.insert(u"include-cursor"_s, true);
    }
    if (flags & ImagePlatformKWin::ScreenShotFlag::IncludeDecoration) {
        options.insert(u"include-decoration"_s, true);
    }
    if (flags & ImagePlatformKWin::ScreenShotFlag::NativeSize) {
        options.insert(u"native-resolution"_s, true);
    }

    return options;
}

static const QString s_screenShotService = u"org.kde.KWin.ScreenShot2"_s;
static const QString s_screenShotObjectPath = u"/org/kde/KWin/ScreenShot2"_s;
static const QString s_screenShotInterface = u"org.kde.KWin.ScreenShot2"_s;

template<typename... ArgType>
ScreenShotSource2::ScreenShotSource2(const QString &methodName, ArgType... arguments)
{
    // Do not set the O_NONBLOCK flag. Code that reads data from the pipe assumes
    // that read() will block if there is no any data yet.
    int pipeFds[2];
    if (pipe2(pipeFds, O_CLOEXEC) == -1) {
        QTimer::singleShot(0, this, &ScreenShotSource2::errorOccurred);
        qWarning() << "pipe2() failed:" << strerror(errno);
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(s_screenShotService, s_screenShotObjectPath, s_screenShotInterface, methodName);

    QVariantList dbusArguments{arguments...};
    dbusArguments.append(QVariant::fromValue(QDBusUnixFileDescriptor(pipeFds[1])));
    message.setArguments(dbusArguments);

    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);
    close(pipeFds[1]);
    m_pipeFileDescriptor = pipeFds[0];

    auto watcher = new QDBusPendingCallWatcher(pendingCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
        watcher->deleteLater();
        const QDBusPendingReply<QVariantMap> reply = *watcher;

        if (reply.isError()) {
            qWarning() << "Screenshot request failed:" << reply.error().message();
            if (reply.error().name() == u"org.kde.KWin.ScreenShot2.Error.Cancelled"_s) {
                // don't show error on user cancellation
                Q_EMIT finished(m_result);
            } else {
                Q_EMIT errorOccurred();
            }
        } else {
            handleMetaDataReceived(reply);
        }
    });
}

ScreenShotSource2::~ScreenShotSource2()
{
    if (m_pipeFileDescriptor != -1) {
        close(m_pipeFileDescriptor);
    }
}

QImage ScreenShotSource2::result() const
{
    return m_result;
}

static QImage allocateImage(const QVariantMap &metadata)
{
    bool ok;

    const uint width = metadata.value(u"width"_s).toUInt(&ok);
    if (!ok) {
        return QImage();
    }

    const uint height = metadata.value(u"height"_s).toUInt(&ok);
    if (!ok) {
        return QImage();
    }

    const uint format = metadata.value(u"format"_s).toUInt(&ok);
    if (!ok) {
        return QImage();
    }

    return QImage(width, height, QImage::Format(format));
}

static QImage readImage(int fileDescriptor, const QVariantMap &metadata)
{
    QFile file;
    if (!file.open(fileDescriptor, QFileDevice::ReadOnly, QFileDevice::AutoCloseHandle)) {
        close(fileDescriptor);
        return QImage();
    }

    QImage result = allocateImage(metadata);
    if (result.isNull()) {
        return QImage();
    }

    const auto windowId = metadata.value(u"windowId"_s).toString();
    // No point in storing the windowId in the image since it means nothing to users
    // and can't be used if the window is closed.
    if (!windowId.isEmpty()) {
        QDBusMessage message = QDBusMessage::createMethodCall(u"org.kde.KWin"_s,
                                                              u"/KWin"_s,
                                                              u"org.kde.KWin"_s,
                                                              u"getWindowInfo"_s);
        message.setArguments({windowId});
        const QDBusReply<QVariantMap> reply = QDBusConnection::sessionBus().call(message);
        if (reply.isValid()) {
            const auto &windowTitle = reply.value().value(u"caption"_s).toString();
            if (!windowTitle.isEmpty()) {
                result.setText(u"windowTitle"_s, windowTitle);
                ExportManager::instance()->setWindowTitle(windowTitle);
            }
        }
    }

    bool ok = false;
    const qreal scale = metadata.value(u"scale"_s).toReal(&ok);
    if (ok) {
        result.setDevicePixelRatio(scale);
    }

    const auto screen = metadata.value(u"screen"_s).toString();
    if (!screen.isEmpty()) {
        result.setText(u"screen"_s, screen);
    }

    QDataStream stream(&file);
    stream.readRawData(reinterpret_cast<char *>(result.bits()), result.sizeInBytes());

    return result;
}

void ScreenShotSource2::handleMetaDataReceived(const QVariantMap &metadata)
{
    const QString type = metadata.value(u"type"_s).toString();
    if (type != "raw"_L1) {
        qWarning() << "Unsupported metadata type:" << type;
        return;
    }

    auto watcher = new QFutureWatcher<QImage>(this);
    connect(watcher, &QFutureWatcher<QImage>::finished, this, [this, watcher]() {
        watcher->deleteLater();
        m_result = watcher->result();
        if (m_result.isNull()) {
            Q_EMIT errorOccurred();
        } else {
            Q_EMIT finished(m_result);
        }
    });
    watcher->setFuture(QtConcurrent::run(readImage, m_pipeFileDescriptor, metadata));

    // The ownership of the pipe file descriptor has been moved to the worker thread.
    m_pipeFileDescriptor = -1;
}

ScreenShotSourceArea2::ScreenShotSourceArea2(const QRect &area, ImagePlatformKWin::ScreenShotFlags flags)
    : ScreenShotSource2(u"CaptureArea"_s,
                        qint32(area.x()),
                        qint32(area.y()),
                        quint32(area.width()),
                        quint32(area.height()),
                        screenShotFlagsToVardict(flags))
{
}

ScreenShotSourceInteractive2::ScreenShotSourceInteractive2(ImagePlatformKWin::InteractiveKind kind, ImagePlatformKWin::ScreenShotFlags flags)
    : ScreenShotSource2(u"CaptureInteractive"_s, quint32(kind), screenShotFlagsToVardict(flags))
{
}

ScreenShotSourceScreen2::ScreenShotSourceScreen2(const QScreen *screen, ImagePlatformKWin::ScreenShotFlags flags)
// NOTE: As of Qt 6.4, QScreen::name() is not guaranteed to match the result of any native APIs.
// It should not be used to uniquely identify a screen, but it happens to work on X11 and Wayland.
// KWin's ScreenShot2 DBus API uses QScreen::name() as identifiers for screens.
    : ScreenShotSource2(u"CaptureScreen"_s, screen->name(), screenShotFlagsToVardict(flags))
{
}

ScreenShotSourceActiveWindow2::ScreenShotSourceActiveWindow2(ImagePlatformKWin::ScreenShotFlags flags)
    : ScreenShotSource2(u"CaptureActiveWindow"_s, screenShotFlagsToVardict(flags))
{
}

ScreenShotSourceActiveScreen2::ScreenShotSourceActiveScreen2(ImagePlatformKWin::ScreenShotFlags flags)
    : ScreenShotSource2(u"CaptureActiveScreen"_s, screenShotFlagsToVardict(flags))
{
}

ScreenShotSourceWorkspace2::ScreenShotSourceWorkspace2(ImagePlatformKWin::ScreenShotFlags flags)
    : ScreenShotSource2(u"CaptureWorkspace"_s, screenShotFlagsToVardict(flags))
{
}

ImagePlatformKWin::ImagePlatformKWin(QObject *parent)
    : ImagePlatform(parent)
{
    auto message = QDBusMessage::createMethodCall(u"org.kde.KWin.ScreenShot2"_s,
                                                  u"/org/kde/KWin/ScreenShot2"_s,
                                                  u"org.freedesktop.DBus.Properties"_s,
                                                  u"Get"_s);
    message.setArguments({u"org.kde.KWin.ScreenShot2"_s, u"Version"_s});

    const QDBusMessage reply = QDBusConnection::sessionBus().call(message);
    if (reply.type() == QDBusMessage::ReplyMessage) {
        m_apiVersion = reply.arguments().constFirst().value<QDBusVariant>().variant().toUInt();
    }

    updateSupportedGrabModes();
    connect(qGuiApp, &QGuiApplication::screenAdded, this, &ImagePlatformKWin::updateSupportedGrabModes);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &ImagePlatformKWin::updateSupportedGrabModes);
}

ImagePlatform::GrabModes ImagePlatformKWin::supportedGrabModes() const
{
    return m_grabModes;
}

void ImagePlatformKWin::updateSupportedGrabModes()
{
    ImagePlatform::GrabModes grabModes = GrabMode::AllScreens | GrabMode::WindowUnderCursor | GrabMode::PerScreenImageNative;

    if (m_apiVersion >= 2) {
        grabModes |= GrabMode::ActiveWindow;
    }

    if (QGuiApplication::screens().count() > 1) {
        grabModes |= GrabMode::CurrentScreen | GrabMode::AllScreensScaled;
    }

    if (m_grabModes != grabModes) {
        m_grabModes = grabModes;
        Q_EMIT supportedGrabModesChanged();
    }
}

ImagePlatform::ShutterModes ImagePlatformKWin::supportedShutterModes() const
{
    return ShutterMode::Immediate;
}

void ImagePlatformKWin::doGrab(ShutterMode, GrabMode grabMode, bool includePointer, bool includeDecorations)
{
    ScreenShotFlags flags = ScreenShotFlag::NativeSize;

    if (includeDecorations) {
        flags |= ScreenShotFlag::IncludeDecoration;
    }
    if (includePointer) {
        flags |= ScreenShotFlag::IncludeCursor;
    }

    switch (grabMode) {
    case GrabMode::AllScreens:
        takeScreenShotWorkspace(flags);
        break;
    case GrabMode::CurrentScreen:
        if (m_apiVersion >= 2) {
            takeScreenShotActiveScreen(flags);
        } else {
            takeScreenShotInteractive(InteractiveKind::Screen, flags);
        }
        break;
    case GrabMode::ActiveWindow:
        takeScreenShotActiveWindow(flags);
        break;
    case GrabMode::TransientWithParent:
    case GrabMode::WindowUnderCursor:
        takeScreenShotInteractive(InteractiveKind::Window, flags);
        break;
    case GrabMode::AllScreensScaled:
        takeScreenShotWorkspace(flags & ~ScreenShotFlags(ScreenShotFlag::NativeSize));
        break;
    case GrabMode::PerScreenImageNative:
        takeScreenShotCroppable(flags);
        break;
    case GrabMode::NoGrabModes:
        Q_EMIT newScreenshotFailed();
        break;
    }
}

void ImagePlatformKWin::trackSource(ScreenShotSource2 *source)
{
    connect(source, &ScreenShotSource2::finished, this, [this, source](const QImage &image) {
        source->deleteLater();
        Q_EMIT newScreenshotTaken(image);
    });
    connect(source, &ScreenShotSource2::errorOccurred, this, [this, source]() {
        source->deleteLater();
        Q_EMIT newScreenshotFailed();
    });
}

void ImagePlatformKWin::trackCroppableSource(ScreenShotSourceWorkspace2 *source)
{
    connect(source, &ScreenShotSourceWorkspace2::finished, this, [this, source](const QImage &image) {
        source->deleteLater();
        Q_EMIT newCroppableScreenshotTaken(image);
    });
    connect(source, &ScreenShotSourceWorkspace2::errorOccurred, this, [this, source]() {
        source->deleteLater();
        Q_EMIT newScreenshotFailed();
    });
}

void ImagePlatformKWin::takeScreenShotArea(const QRect &area, ScreenShotFlags flags)
{
    trackSource(new ScreenShotSourceArea2(area, flags));
}

void ImagePlatformKWin::takeScreenShotInteractive(InteractiveKind kind, ScreenShotFlags flags)
{
    trackSource(new ScreenShotSourceInteractive2(kind, flags));
}

void ImagePlatformKWin::takeScreenShotActiveWindow(ScreenShotFlags flags)
{
    trackSource(new ScreenShotSourceActiveWindow2(flags));
}

void ImagePlatformKWin::takeScreenShotActiveScreen(ScreenShotFlags flags)
{
    trackSource(new ScreenShotSourceActiveScreen2(flags));
}

void ImagePlatformKWin::takeScreenShotWorkspace(ScreenShotFlags flags)
{
    trackSource(new ScreenShotSourceWorkspace2(flags));
}

void ImagePlatformKWin::takeScreenShotCroppable(ScreenShotFlags flags)
{
    trackCroppableSource(new ScreenShotSourceWorkspace2(flags));
}

#include "moc_ImagePlatformKWin.cpp"
