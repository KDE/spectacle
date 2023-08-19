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

ScreenShotSourceMeta2::ScreenShotSourceMeta2(const QVector<ScreenShotSource2 *> &sources)
    : m_sources(sources)
{
    for (ScreenShotSource2 *source : sources) {
        source->setParent(this);

        connect(source, &ScreenShotSource2::finished, this, &ScreenShotSourceMeta2::handleSourceFinished);
        connect(source, &ScreenShotSource2::errorOccurred, this, &ScreenShotSourceMeta2::handleSourceErrorOccurred);
    }
}

void ScreenShotSourceMeta2::handleSourceFinished()
{
    const bool isFinished = std::all_of(m_sources.constBegin(), m_sources.constEnd(), [](const ScreenShotSource2 *source) {
        return !source->result().isNull();
    });
    if (!isFinished) {
        return;
    }

    QVector<QImage> results;
    results.reserve(m_sources.count());

    for (const ScreenShotSource2 *source : std::as_const(m_sources)) {
        results.append(source->result());
    }

    Q_EMIT finished(results);
}

void ScreenShotSourceMeta2::handleSourceErrorOccurred()
{
    Q_EMIT errorOccurred();
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
        takeScreenShotScreens(QGuiApplication::screens(), flags);
        break;
    case GrabMode::NoGrabModes:
        Q_EMIT newScreenshotFailed();
        break;
    }
}

void PlatformKWin::trackSource(ScreenShotSource2 *source)
{
    connect(source, &ScreenShotSourceArea2::finished, this, [this, source](const QImage &image) {
        source->deleteLater();
        Q_EMIT newScreenshotTaken(image);
    });
    connect(source, &ScreenShotSourceArea2::errorOccurred, this, [this, source]() {
        source->deleteLater();
        Q_EMIT newScreenshotFailed();
    });
}

void PlatformKWin::trackSource(ScreenShotSourceMeta2 *source)
{
    connect(source, &ScreenShotSourceMeta2::finished, this, [this, source](const QVector<QImage> &images) {
        source->deleteLater();
        QVector<CanvasImage> screenImages;
        const auto &screens = qGuiApp->screens();
        QStringList missingScreens;
        for (int i = 0; i < images.size(); ++i) {
            auto &image = images[i];
            const auto screenName = image.text(QStringLiteral("screen"));
            if (screenName.isEmpty()) {
                qWarning() << "ERROR: A screen image did not have an associated screen name.";
                Q_EMIT newScreenshotFailed();
                return;
            }

            bool screenFound = false;
            for (int ii = 0; ii < screens.size(); ++ii) {
                auto screen = screens[ii];
                screenFound |= screen->name() == screenName;
                if (screenFound) {
                    QRectF screenRect = screen->geometry();
                    // On X11 QScreen::geometry has a scaled and rounded size, but a raw position.
                    // Do additional processing for X11 to get the logical (wayland) geometry.
                    if (KWindowSystem::isPlatformX11()) {
                        // Assume that all screens on X11 have the same DPR.
                        // Plasma System Settings only allows global scaling on X11
                        // and this makes the math easier.
                        qreal scale = image.devicePixelRatio();
                        if (scale == 1 && scale != screen->devicePixelRatio()) {
                            scale = screen->devicePixelRatio();
                        }
                        screenRect.setSize(QSizeF(image.size()) / scale);
                        screenRect.moveTo(screenRect.topLeft() / scale);
                    }
                    screenImages.append({image, screenRect});
                    break;
                }
            }

            if (!screenFound) {
                missingScreens << screenName;
            }
        }
        if (!missingScreens.empty()) {
            qWarning() << "ERROR: The following captured screen names could not be found:"
                       << missingScreens.join(QStringLiteral(", "));
            Q_EMIT newScreenshotFailed();
            return;
        }
        Q_EMIT newScreensScreenshotTaken(screenImages);
    });
    connect(source, &ScreenShotSourceMeta2::errorOccurred, this, [this, source]() {
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

void PlatformKWin::takeScreenShotScreens(const QList<QScreen *> &screens, ScreenShotFlags flags)
{
    QVector<ScreenShotSource2 *> sources;
    sources.reserve(screens.count());

    for (QScreen *screen : screens) {
        sources.append(new ScreenShotSourceScreen2(screen, flags));
    }

    trackSource(new ScreenShotSourceMeta2(sources));
}

#include "moc_PlatformKWin.cpp"
