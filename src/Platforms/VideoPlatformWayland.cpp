/*
    SPDX-FileCopyrightText: 2023 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "VideoPlatformWayland.h"
#include "ExportManager.h"
#include "Platforms/VideoPlatform.h"
#include "screencasting.h"
#include "settings.h"
#include <KLocalizedString>
#include <KMemoryInfo>
#include <QFuture>
#include <QGuiApplication>
#include <QWindow>
#include <QScreen>
#include <QDebug>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUrl>
#include <QtConcurrentRun>

using namespace Qt::StringLiterals;

using Format = VideoPlatform::Format;
using Formats = VideoPlatform::Formats;
using Encoder = PipeWireBaseEncodedStream::Encoder;

static const auto screenKey = u"screen"_s;
static const auto windowIdKey = u"uuid"_s;
static const auto rectKey = u"rect"_s;
static const auto xKey = u"x"_s;
static const auto yKey = u"y"_s;
static const auto widthKey = u"width"_s;
static const auto heightKey = u"height"_s;
static const auto captionKey = u"caption"_s;
static const auto desktopFileKey = u"desktopFile"_s;

static constexpr inline int frameBytes(const QSize &frameSize)
{
    // It is currently not possible to do 10 or 12 bit per color channel
    // recording so we always use 4 bytes (8 bits * 4 channels).
    return std::max(0, frameSize.width() * frameSize.height() * 4);
}

static inline int availableFrames(int frameBytes){
    KMemoryInfo info;
    if (info.isNull() || frameBytes <= 0) {
        // The default max frame buffer is 50.
        return 50;
    }
    // Reduce the amount of available memory we try to use to 75% because we
    // really don't want to freeze the user's computer.
    // The minimum max frame buffer is 3. It's used automatically when the buffer
    // is less than 3, but that sends out warning messages, so we explicitly set
    // 3 to prevent warning spam.
    return std::max(3.0, 0.75 * info.availablePhysical() / frameBytes);
}

VideoPlatform::Format VideoPlatformWayland::formatForEncoder(Encoder encoder) const
{
    switch (encoder) {
    case Encoder::VP9: return WebM_VP9;
    case Encoder::H264Main: return MP4_H264;
    case Encoder::H264Baseline: return MP4_H264;
    case Encoder::WebP: return WebP;
    case Encoder::Gif: return Gif;
    default: return NoFormat;
    }
}

PipeWireBaseEncodedStream::Encoder VideoPlatformWayland::encoderForFormat(Format format) const
{
    const auto encoders = m_recorder ? m_recorder->suggestedEncoders() : QList<Encoder>{};
    if (format == WebM_VP9 && encoders.contains(Encoder::VP9)) {
        return Encoder::VP9;
    }
    if (format == MP4_H264) {
        if (encoders.contains(Encoder::H264Main)) {
            return Encoder::H264Main;
        }
        if (encoders.contains(Encoder::H264Baseline)) {
            return Encoder::H264Baseline;
        }
    }
    if (format == WebP && encoders.contains(Encoder::WebP)) {
        return Encoder::WebP;
    }
    if (format == Gif && encoders.contains(Encoder::Gif)) {
        return Encoder::Gif;
    }
    return Encoder::NoEncoder;
}

static void minimizeIfWindowsIntersect(const QRectF &rect) {
    if (rect.isEmpty()) {
        return;
    }
    const auto &windows = qGuiApp->allWindows();
    for (auto window : windows) {
        if (rect.intersects(window->frameGeometry())
            && window->isVisible() && window->visibility() != QWindow::Minimized) {
            if (window->visibility() == QWindow::FullScreen) {
                window->setVisible(false);
            }
            window->showMinimized();
        }
    }
}

VideoPlatformWayland::VideoPlatformWayland(QObject *parent)
    : VideoPlatform(parent)
    , m_screencasting(new Screencasting(this))
{
    QMetaObject::invokeMethod(this, &VideoPlatformWayland::initialize, Qt::QueuedConnection);
}

void VideoPlatformWayland::initialize()
{
    m_recorder = std::make_unique<PipeWireRecord>();
    Q_EMIT supportedRecordingModesChanged();
    Q_EMIT supportedFormatsChanged();
}

VideoPlatform::RecordingModes VideoPlatformWayland::supportedRecordingModes() const
{
    if (m_screencasting->isAvailable() && m_recorder)
        return Screen | Window | Region;
    else
        return {};
}

VideoPlatform::Formats VideoPlatformWayland::supportedFormats() const
{
    Formats formats;
    if (m_screencasting->isAvailable() && m_recorder) {
        const auto encoders = m_recorder->suggestedEncoders();
        for (auto encoder : encoders) {
            formats |= formatForEncoder(encoder);
        }
    }
    return formats;
}

void VideoPlatformWayland::startRecording(const QUrl &fileUrl, RecordingMode recordingMode, const QVariantMap &options, bool includePointer)
{
    if (recordingMode == NoRecordingModes) {
        // We should avoid calling startRecording without a recording mode,
        // but it shouldn't cause a runtime error if we can handle it gracefully.
        Q_EMIT recordingCanceled(u"Recording canceled: No recording mode"_s);
        return;
    }
    if (!m_screencasting->isAvailable()) {
        Q_EMIT recordingFailed(i18nc("@info", "KWin Screencasting is not available."));
        return;
    }
    m_recorderFuture.waitForFinished();
    if (recordingState() == RecordingState::Recording) {
        qWarning() << "Warning: Tried to start recording while already recording.";
        return;
    }
    if (recordingState() == RecordingState::Rendering) {
        qWarning() << "Warning: Tried to start recording while already rendering.";
        return;
    }
    if (!fileUrl.isEmpty() && !fileUrl.isLocalFile()) {
        Q_EMIT recordingFailed(i18nc("@info:shell", "Failed to record: File URL is not a local file"));
        return;
    }

    Screencasting::CursorMode mode = includePointer ? Screencasting::CursorMode::Embedded : Screencasting::Hidden;
    ScreencastingStream *stream = nullptr;
    switch (recordingMode) {
    case Screen: {
        auto screen = options.value(screenKey).value<QScreen *>();
        if (!screen) {
            selectAndRecord(fileUrl, recordingMode, includePointer);
            return;
        }
        Q_ASSERT(screen != nullptr);
        minimizeIfWindowsIntersect(screen->geometry());
        m_frameBytes = frameBytes(screen->size() * screen->devicePixelRatio());
        stream = m_screencasting->createOutputStream(screen, mode);
        break;
    }
    case Window: {
        auto windowId = options.value(windowIdKey).toString();
        if (windowId.isEmpty()) {
            selectAndRecord(fileUrl, recordingMode, includePointer);
            return;
        }
        Q_ASSERT(!windowId.isEmpty());
        // HACK: Window geometry from queryWindowInfo is from KWin's Window::frameGeometry(),
        // which may not be the same as QWindow::frameGeometry() on Wayland.
        // Hopefully this is good enough most of the time.
        const QRectF windowRect{
            options[xKey].toDouble(), options[yKey].toDouble(),
            options[widthKey].toDouble(), options[heightKey].toDouble(),
        };
        const bool isSpectacle = options[desktopFileKey].toString() == qGuiApp->desktopFileName();
        if (!windowRect.isEmpty() && !isSpectacle) {
            minimizeIfWindowsIntersect(windowRect);
        }
        auto screen = qGuiApp->screenAt(windowRect.center().toPoint());
        m_frameBytes = frameBytes(windowRect.size().toSize() * (screen ? screen->devicePixelRatio() : 1));
        stream = m_screencasting->createWindowStream(windowId, mode);
        break;
    }
    case Region: {
        auto rect = options.value(rectKey).toRectF();
        if (rect.isEmpty()) {
            selectAndRecord(fileUrl, recordingMode, includePointer);
            return;
        }
        qreal scaling = 1;
        const auto screens = qGuiApp->screens();
        // Don't make the resolution larger than it needs to be.
        for (auto screen : screens) {
            if (rect.intersects(screen->geometry())) {
                scaling = std::max(scaling, screen->devicePixelRatio());
            }
        }
        // Round to pixels that fit within the original rect.
        // We do this to make it easier to keep the selection outline outside of the recording.
        int x = std::ceil(rect.x());
        int y = std::ceil(rect.y());
        // We calculate size using floor(right) - x and floor(bottom) - y so that the rect doesn't
        // shift too much. Ensure size is at least 1x1 to keep the final rect valid.
        int w = std::max(1.0, std::floor(rect.right()) - x);
        int h = std::max(1.0, std::floor(rect.bottom()) - y);
        m_frameBytes = frameBytes(QSize{w, h} * scaling);
        scaling = m_screencasting->isRegionAutoScaleSupported() ? 0 : scaling;
        stream = m_screencasting->createRegionStream({x, y, w, h}, scaling, mode);
        break;
    }
    default: break; // This shouldn't happen
    }
    m_recorder->setMaxPendingFrames(availableFrames(m_frameBytes));
    m_recorder->setNodeId(0);

    Q_ASSERT(stream);
    connect(stream, &ScreencastingStream::created, this, [this, stream] {
        m_recorder->setNodeId(stream->nodeId());
        if (!m_recorder->output().isEmpty()) {
            m_recorder->start();
        }
        setRecordingState(VideoPlatform::RecordingState::Recording);
    });
    connect(stream, &ScreencastingStream::failed, this, [this](const QString &error) {
        setRecordingState(VideoPlatform::RecordingState::NotRecording);
        Q_EMIT recordingFailed(error);
    });
    connect(stream, &ScreencastingStream::closed, this, [this, recordingMode]() {
        finishRecording();
        setRecordingState(VideoPlatform::RecordingState::Finished);
        if (recordingMode == Screen) {
            Q_EMIT recordingFailed(i18nc("@info", "The stream closed because the target screen changed in a way that disrupted the recording."));
        } else if (recordingMode == Window) {
            Q_EMIT recordingFailed(i18nc("@info", "The stream closed because the target window changed in a way that disrupted the recording."));
        } else if (recordingMode == Region) {
            Q_EMIT recordingFailed(i18nc("@info", "The stream closed because a screen containing the target region changed in a way that disrupted the recording."));
        }
    });

    // set up output
    if (!fileUrl.isValid()) {
        ExportManager::instance()->updateTimestamp();
        const auto format = static_cast<Format>(Settings::preferredVideoFormat());
        const auto filename = ExportManager::formattedFilename(Settings::videoFilenameTemplate(),
                                                               ExportManager::instance()->timestamp(),
                                                               options[captionKey].toString(),
                                                               Settings::videoSaveLocation());
        auto tempUrl = ExportManager::instance()->tempVideoUrl(filename);
        if (!tempUrl.isLocalFile()) {
            Q_EMIT recordingFailed(i18nc("@info:shell", "Failed to record: Temporary file URL is not a local file (%1)", tempUrl.toString()));
            return;
        }
        if (!mkDirPath(tempUrl)) {
            return;
        }
        m_recorder->setEncoder(encoderForFormat(format));
        m_recorder->setOutput(tempUrl.toLocalFile());
    } else {
        if (!fileUrl.isLocalFile()) {
            Q_EMIT recordingFailed(i18nc("@info:shell", "Failed to record: Output file URL is not a local file (%1)", fileUrl.toString()));
            return;
        }
        if (!mkDirPath(fileUrl)) {
            return;
        }
        const auto &localFile = fileUrl.toLocalFile();
        m_recorder->setEncoder(encoderForFormat(formatForPath(localFile)));
        m_recorder->setOutput(localFile);
    }
    if (m_recorder->nodeId() != 0) {
        m_recorder->start();
    }

    connect(m_recorder.get(), &PipeWireRecord::stateChanged, this, [this] {
        if (m_recorder->state() == PipeWireRecord::Idle) {
            m_memoryTimer.stop();
            if (recordingState() != RecordingState::NotRecording && recordingState() != RecordingState::Finished) {
                setRecordingState(VideoPlatform::RecordingState::Finished);
                Q_EMIT recordingSaved(QUrl::fromLocalFile(m_recorder->output()));
            }
        } else if (m_recorder->state() == PipeWireRecord::Recording) {
            m_memoryTimer.start(5000, Qt::CoarseTimer, this);
            setRecordingState(VideoPlatform::RecordingState::Recording);
        } else if (m_recorder->state() == PipeWireRecord::Rendering) {
            m_memoryTimer.stop();
            setRecordingState(VideoPlatform::RecordingState::Rendering);
        }
    });
}

void VideoPlatformWayland::finishRecording()
{
    if (!m_recorder) {
        return;
    }
    m_recorder->stop();
}

void VideoPlatformWayland::timerEvent(QTimerEvent *event)
{
    VideoPlatform::timerEvent(event);
    if (event->timerId() == m_memoryTimer.timerId() && m_recorder) {
        m_recorder->setMaxPendingFrames(availableFrames(m_frameBytes));
    }
}

bool VideoPlatformWayland::mkDirPath(const QUrl &fileUrl)
{
    QDir dir(fileUrl.adjusted(QUrl::RemoveFilename).toLocalFile());
    if (dir.exists() || dir.mkpath(u"."_s)) {
        return true;
    } else {
        Q_EMIT recordingFailed(i18nc("@info:shell", "Failed to record: Unable to create folder (%1)", dir.path()));
        return false;
    }
}

void VideoPlatformWayland::selectAndRecord(const QUrl &fileUrl, RecordingMode recordingMode, bool includePointer)
{
    if (recordingMode == Region) {
        Q_EMIT regionRequested();
        return;
    }

    // We should probably come up with a better way of choosing outputs. This should be okay for now. #FLW
    QDBusMessage message = QDBusMessage::createMethodCall(u"org.kde.KWin"_s,
                                                          u"/KWin"_s,
                                                          u"org.kde.KWin"_s,
                                                          u"queryWindowInfo"_s);

    QDBusPendingReply<QVariantMap> asyncReply = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(asyncReply, this);
    auto onFinished = [this, fileUrl, recordingMode, includePointer](QDBusPendingCallWatcher *self) {
        QDBusPendingReply<QVariantMap> reply = *self;
        self->deleteLater();
        if (!reply.isValid()) {
            const auto &error = self->error();
            if (error.name() == u"org.kde.KWin.Error.UserCancel"_s) {
                QString message;
                if (recordingMode == Screen) {
                    message = i18nc("@info:shell", "Screen recording canceled");
                } else {
                    message = i18nc("@info:shell", "Window recording canceled");
                }
                Q_EMIT recordingCanceled(message);
            } else {
                QString message;
                if (recordingMode == Screen) {
                    message = i18nc("@info:shell", "Failed to select screen: %1", error.message());
                } else {
                    message = i18nc("@info:shell", "Failed to select window: %1", error.message());
                }
                Q_EMIT recordingFailed(message);
            }
            return;
        }
        const auto &data = reply.value();
        QVariantMap options;
        if (recordingMode == Screen) {
            QPoint pos = QCursor::pos();
            // BUG: https://bugs.kde.org/show_bug.cgi?id=480599
            // On wayland, you can't always get the cursor position from QCursor::pos().
            // However, using selected window geometry can sometimes select the wrong screen if the
            // window is between screens. We'll need to come up with a better solution someday.
            if (pos.isNull()) {
                pos = {
                    data[xKey].toInt() + data[widthKey].toInt() / 2,
                    data[yKey].toInt() + data[heightKey].toInt() / 2};
            }
            const auto &screens = qGuiApp->screens();
            QScreen *screen = nullptr;
            for (auto s : screens) {
                if (s->geometry().contains(pos)) {
                    screen = s;
                    break;
                }
            }
            if (!screen) {
                Q_EMIT recordingFailed(i18nc("@info:shell", "Failed to select screen: No screen contained the mouse cursor position"));
                return;
            }
            options[screenKey] = QVariant::fromValue(screen);
        } else {
            const auto &windowId = data.value(windowIdKey).toString();
            if (windowId.isEmpty()) {
                Q_EMIT recordingFailed(i18nc("@info:shell", "Failed to select window: No window found"));
                return;
            }
            options = data;
        }
        startRecording(fileUrl, recordingMode, options, includePointer);
    };
    connect(watcher, &QDBusPendingCallWatcher::finished, this, onFinished);
}

#include "moc_VideoPlatformWayland.cpp"
