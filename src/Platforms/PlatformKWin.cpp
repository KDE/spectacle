/*
    SPDX-FileCopyrightText: 2021 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "PlatformKWin.h"
#include "ExportManager.h"

#include <KWindowSystem>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>
#include <QFuture>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QPixmap>
#include <QScreen>
#include <QtConcurrent>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

static QVariantMap screenShotFlagsToVardict(PlatformKWin::ScreenShotFlags flags)
{
    QVariantMap options;

    if (flags & PlatformKWin::ScreenShotFlag::IncludeCursor) {
        options.insert(QStringLiteral("include-cursor"), true);
    }
    if (flags & PlatformKWin::ScreenShotFlag::IncludeDecoration) {
        options.insert(QStringLiteral("include-decoration"), true);
    }
    if (flags & PlatformKWin::ScreenShotFlag::NativeSize) {
        options.insert(QStringLiteral("native-resolution"), true);
    }

    return options;
}

static const QString s_screenShotService = QStringLiteral("org.kde.KWin.ScreenShot2");
static const QString s_screenShotObjectPath = QStringLiteral("/org/kde/KWin/ScreenShot2");
static const QString s_screenShotInterface = QStringLiteral("org.kde.KWin.ScreenShot2");

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
            if (reply.error().name() == QStringLiteral("org.kde.KWin.ScreenShot2.Error.Cancelled")) {
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

    const uint width = metadata.value(QStringLiteral("width")).toUInt(&ok);
    if (!ok) {
        return QImage();
    }

    const uint height = metadata.value(QStringLiteral("height")).toUInt(&ok);
    if (!ok) {
        return QImage();
    }

    const uint format = metadata.value(QStringLiteral("format")).toUInt(&ok);
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

    const auto windowId = metadata.value(QStringLiteral("windowId")).toString();
    // No point in storing the windowId in the image since it means nothing to users
    // and can't be used if the window is closed.
    if (!windowId.isEmpty()) {
        QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
                                                              QStringLiteral("/KWin"),
                                                              QStringLiteral("org.kde.KWin"),
                                                              QStringLiteral("getWindowInfo"));
        message.setArguments({windowId});
        const QDBusReply<QVariantMap> reply = QDBusConnection::sessionBus().call(message);
        if (reply.isValid()) {
            const auto &windowTitle = reply.value().value(QStringLiteral("caption")).toString();
            if (!windowTitle.isEmpty()) {
                result.setText(QStringLiteral("windowTitle"), windowTitle);
                ExportManager::instance()->setWindowTitle(windowTitle);
            }
        }
    }

    bool ok = false;
    const qreal scale = metadata.value(QStringLiteral("scale")).toReal(&ok);
    if (ok) {
        result.setDevicePixelRatio(scale);
    }

    const auto screen = metadata.value(QStringLiteral("screen")).toString();
    if (!screen.isEmpty()) {
        result.setText(QStringLiteral("screen"), screen);
    }

    QDataStream stream(&file);
    stream.readRawData(reinterpret_cast<char *>(result.bits()), result.sizeInBytes());

    return result;
}

void ScreenShotSource2::handleMetaDataReceived(const QVariantMap &metadata)
{
    const QString type = metadata.value(QStringLiteral("type")).toString();
    if (type != QLatin1String("raw")) {
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

ScreenShotSourceArea2::ScreenShotSourceArea2(const QRect &area, PlatformKWin::ScreenShotFlags flags)
    : ScreenShotSource2(QStringLiteral("CaptureArea"),
                        qint32(area.x()),
                        qint32(area.y()),
                        quint32(area.width()),
                        quint32(area.height()),
                        screenShotFlagsToVardict(flags))
{
}

ScreenShotSourceInteractive2::ScreenShotSourceInteractive2(PlatformKWin::InteractiveKind kind, PlatformKWin::ScreenShotFlags flags)
    : ScreenShotSource2(QStringLiteral("CaptureInteractive"), quint32(kind), screenShotFlagsToVardict(flags))
{
}

ScreenShotSourceScreen2::ScreenShotSourceScreen2(const QScreen *screen, PlatformKWin::ScreenShotFlags flags)
// NOTE: As of Qt 6.4, QScreen::name() is not guaranteed to match the result of any native APIs.
// It should not be used to uniquely identify a screen, but it happens to work on X11 and Wayland.
// KWin's ScreenShot2 DBus API uses QScreen::name() as identifiers for screens.
    : ScreenShotSource2(QStringLiteral("CaptureScreen"), screen->name(), screenShotFlagsToVardict(flags))
{
}

ScreenShotSourceActiveWindow2::ScreenShotSourceActiveWindow2(PlatformKWin::ScreenShotFlags flags)
    : ScreenShotSource2(QStringLiteral("CaptureActiveWindow"), screenShotFlagsToVardict(flags))
{
}

ScreenShotSourceActiveScreen2::ScreenShotSourceActiveScreen2(PlatformKWin::ScreenShotFlags flags)
    : ScreenShotSource2(QStringLiteral("CaptureActiveScreen"), screenShotFlagsToVardict(flags))
{
}

std::unique_ptr<PlatformKWin> PlatformKWin::create()
{
    QDBusConnectionInterface *interface = QDBusConnection::sessionBus().interface();
    if (interface->isServiceRegistered(s_screenShotService)) {
        return std::unique_ptr<PlatformKWin>(new PlatformKWin());
    }
    return nullptr;
}

PlatformKWin::PlatformKWin(QObject *parent)
    : Platform(parent)
{
    auto message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin.ScreenShot2"),
                                                  QStringLiteral("/org/kde/KWin/ScreenShot2"),
                                                  QStringLiteral("org.freedesktop.DBus.Properties"),
                                                  QStringLiteral("Get"));
    message.setArguments({QStringLiteral("org.kde.KWin.ScreenShot2"), QStringLiteral("Version")});

    const QDBusMessage reply = QDBusConnection::sessionBus().call(message);
    if (reply.type() == QDBusMessage::ReplyMessage) {
        m_apiVersion = reply.arguments().constFirst().value<QDBusVariant>().variant().toUInt();
    }

    updateSupportedGrabModes();
    connect(qGuiApp, &QGuiApplication::screenAdded, this, &PlatformKWin::updateSupportedGrabModes);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &PlatformKWin::updateSupportedGrabModes);
}

Platform::GrabModes PlatformKWin::supportedGrabModes() const
{
    return m_grabModes;
}

void PlatformKWin::updateSupportedGrabModes()
{
    Platform::GrabModes grabModes = GrabMode::AllScreens | GrabMode::WindowUnderCursor | GrabMode::PerScreenImageNative;

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

Platform::ShutterModes PlatformKWin::supportedShutterModes() const
{
    return ShutterMode::Immediate;
}

static QRect workArea()
{
    const QList<QScreen *> screens = QGuiApplication::screens();

    auto accumulateFunc = [](const QRect &accumulator, const QScreen *screen) {
        return accumulator.united(screen->geometry());
    };

    return std::accumulate(screens.begin(), screens.end(), QRect(), accumulateFunc);
}

void PlatformKWin::doGrab(ShutterMode, GrabMode grabMode, bool includePointer, bool includeDecorations)
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
        takeScreenShotArea(workArea(), flags);
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
        takeScreenShotArea(workArea(), flags & ~ScreenShotFlags(ScreenShotFlag::NativeSize));
        break;
    case GrabMode::PerScreenImageNative:
        takeScreenShotCroppable(flags);
        break;
    case GrabMode::NoGrabModes:
        Q_EMIT newScreenshotFailed();
        break;
    }
}

void PlatformKWin::trackSource(ScreenShotSource2 *source)
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

void PlatformKWin::trackCroppableSource(ScreenShotSourceArea2 *source)
{
    connect(source, &ScreenShotSourceArea2::finished, this, [this, source](const QImage &image) {
        source->deleteLater();
        Q_EMIT newCroppableScreenshotTaken(image);
    });
    connect(source, &ScreenShotSourceArea2::errorOccurred, this, [this, source]() {
        source->deleteLater();
        Q_EMIT newScreenshotFailed();
    });
}

void PlatformKWin::takeScreenShotArea(const QRect &area, ScreenShotFlags flags)
{
    trackSource(new ScreenShotSourceArea2(area, flags));
}

void PlatformKWin::takeScreenShotInteractive(InteractiveKind kind, ScreenShotFlags flags)
{
    trackSource(new ScreenShotSourceInteractive2(kind, flags));
}

void PlatformKWin::takeScreenShotActiveWindow(ScreenShotFlags flags)
{
    trackSource(new ScreenShotSourceActiveWindow2(flags));
}

void PlatformKWin::takeScreenShotActiveScreen(ScreenShotFlags flags)
{
    trackSource(new ScreenShotSourceActiveScreen2(flags));
}

void PlatformKWin::takeScreenShotCroppable(ScreenShotFlags flags)
{
    trackCroppableSource(new ScreenShotSourceArea2(workArea(), flags));
}

#include "moc_PlatformKWin.cpp"
