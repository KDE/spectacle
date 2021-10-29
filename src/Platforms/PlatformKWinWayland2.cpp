/*
    SPDX-FileCopyrightText: 2021 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "PlatformKWinWayland2.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusUnixFileDescriptor>
#include <QFuture>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QPixmap>
#include <QtConcurrent>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

static QVariantMap screenShotFlagsToVardict(PlatformKWinWayland2::ScreenShotFlags flags)
{
    QVariantMap options;

    if (flags & PlatformKWinWayland2::ScreenShotFlag::IncludeCursor) {
        options.insert(QStringLiteral("include-cursor"), true);
    }
    if (flags & PlatformKWinWayland2::ScreenShotFlag::IncludeDecoration) {
        options.insert(QStringLiteral("include-decoration"), true);
    }
    if (flags & PlatformKWinWayland2::ScreenShotFlag::NativeSize) {
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

ScreenShotSourceArea2::ScreenShotSourceArea2(const QRect &area, PlatformKWinWayland2::ScreenShotFlags flags)
    : ScreenShotSource2(QStringLiteral("CaptureArea"),
                        qint32(area.x()),
                        qint32(area.y()),
                        quint32(area.width()),
                        quint32(area.height()),
                        screenShotFlagsToVardict(flags))
{
}

ScreenShotSourceInteractive2::ScreenShotSourceInteractive2(PlatformKWinWayland2::InteractiveKind kind, PlatformKWinWayland2::ScreenShotFlags flags)
    : ScreenShotSource2(QStringLiteral("CaptureInteractive"), quint32(kind), screenShotFlagsToVardict(flags))
{
}

ScreenShotSourceScreen2::ScreenShotSourceScreen2(const QScreen *screen, PlatformKWinWayland2::ScreenShotFlags flags)
    : ScreenShotSource2(QStringLiteral("CaptureScreen"), screen->name(), screenShotFlagsToVardict(flags))
{
}

ScreenShotSourceActiveWindow2::ScreenShotSourceActiveWindow2(PlatformKWinWayland2::ScreenShotFlags flags)
    : ScreenShotSource2(QStringLiteral("CaptureActiveWindow"), screenShotFlagsToVardict(flags))
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

std::unique_ptr<PlatformKWinWayland2> PlatformKWinWayland2::create()
{
    QDBusConnectionInterface *interface = QDBusConnection::sessionBus().interface();
    if (interface->isServiceRegistered(s_screenShotService)) {
        return std::unique_ptr<PlatformKWinWayland2>(new PlatformKWinWayland2());
    }
    return nullptr;
}

PlatformKWinWayland2::PlatformKWinWayland2(QObject *parent)
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
}

QString PlatformKWinWayland2::platformName() const
{
    return QStringLiteral("PlatformKWinWayland2");
}

Platform::GrabModes PlatformKWinWayland2::supportedGrabModes() const
{
    Platform::GrabModes supportedModes = GrabMode::AllScreens | GrabMode::WindowUnderCursor | GrabMode::PerScreenImageNative | GrabMode::AllScreensScaled;

    if (m_apiVersion >= 2) {
        supportedModes |= GrabMode::ActiveWindow;
    }

    const QList<QScreen *> screens = QGuiApplication::screens();
    if (screens.count() > 1) {
        supportedModes |= GrabMode::CurrentScreen;
    }

    return supportedModes;
}

Platform::ShutterModes PlatformKWinWayland2::supportedShutterModes() const
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

void PlatformKWinWayland2::doGrab(ShutterMode, GrabMode theGrabMode, bool theIncludePointer, bool theIncludeDecorations)
{
    ScreenShotFlags flags = ScreenShotFlag::NativeSize;

    if (theIncludeDecorations) {
        flags |= ScreenShotFlag::IncludeDecoration;
    }
    if (theIncludePointer) {
        flags |= ScreenShotFlag::IncludeCursor;
    }

    switch (theGrabMode) {
    case GrabMode::AllScreens:
        takeScreenShotArea(workArea(), flags);
        break;
    case GrabMode::CurrentScreen:
        takeScreenShotInteractive(InteractiveKind::Screen, flags);
        break;
    case GrabMode::ActiveWindow:
        takeScreenShotActiveWindow(flags);
        break;
    case GrabMode::WindowUnderCursor:
        takeScreenShotInteractive(InteractiveKind::Window, flags);
        break;
    case GrabMode::AllScreensScaled:
        takeScreenShotArea(workArea(), flags & ~ScreenShotFlags(ScreenShotFlag::NativeSize));
        break;
    case GrabMode::PerScreenImageNative:
        takeScreenShotScreens(QGuiApplication::screens(), flags);
        break;

    case GrabMode::InvalidChoice:
    case GrabMode::TransientWithParent:
        Q_EMIT newScreenshotFailed();
        break;
    }
}

void PlatformKWinWayland2::trackSource(ScreenShotSource2 *source)
{
    connect(source, &ScreenShotSourceArea2::finished, this, [this, source](const QImage &image) {
        source->deleteLater();
        Q_EMIT newScreenshotTaken(QPixmap::fromImage(image));
    });
    connect(source, &ScreenShotSourceArea2::errorOccurred, this, [this, source]() {
        source->deleteLater();
        Q_EMIT newScreenshotFailed();
    });
}

void PlatformKWinWayland2::trackSource(ScreenShotSourceMeta2 *source)
{
    connect(source, &ScreenShotSourceMeta2::finished, this, [this, source](const QVector<QImage> &images) {
        source->deleteLater();
        Q_EMIT newScreensScreenshotTaken(images);
    });
    connect(source, &ScreenShotSourceMeta2::errorOccurred, this, [this, source]() {
        source->deleteLater();
        Q_EMIT newScreenshotFailed();
    });
}

void PlatformKWinWayland2::takeScreenShotArea(const QRect &area, ScreenShotFlags flags)
{
    trackSource(new ScreenShotSourceArea2(area, flags));
}

void PlatformKWinWayland2::takeScreenShotInteractive(InteractiveKind kind, ScreenShotFlags flags)
{
    trackSource(new ScreenShotSourceInteractive2(kind, flags));
}

void PlatformKWinWayland2::takeScreenShotActiveWindow(ScreenShotFlags flags)
{
    trackSource(new ScreenShotSourceActiveWindow2(flags));
}

void PlatformKWinWayland2::takeScreenShotScreens(const QList<QScreen *> &screens, ScreenShotFlags flags)
{
    QVector<ScreenShotSource2 *> sources;
    sources.reserve(screens.count());

    for (QScreen *screen : screens) {
        sources.append(new ScreenShotSourceScreen2(screen, flags));
    }

    trackSource(new ScreenShotSourceMeta2(sources));
}
