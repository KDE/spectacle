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

VideoPlatform::Format VideoPlatformWayland::formatForEncoder(Encoder encoder) const
{
    switch (encoder) {
    case Encoder::VP9: return WebM_VP9;
    case Encoder::H264Main: return MP4_H264;
    case Encoder::H264Baseline: return MP4_H264;
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
    m_recorderFuture = QtConcurrent::run([] {
        return new PipeWireRecord();
    }).then([this](PipeWireRecord *result) {
        m_recorder.reset(result);
        m_recorder->setActive(false);
        Q_EMIT supportedRecordingModesChanged();
        Q_EMIT supportedFormatsChanged();
    });
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

static void setWindowInfo(const QVariantMap &data, QRectF &windowRect, bool &isSpectacle)
{
    // HACK: Window geometry from queryWindowInfo is from KWin's Window::frameGeometry(),
    // which may not be the same as QWindow::frameGeometry() on Wayland.
    // Hopefully this is good enough most of the time.
    windowRect = {
        data[u"x"_s].toDouble(), data[u"y"_s].toDouble(),
        data[u"width"_s].toDouble(), data[u"height"_s].toDouble(),
    };
    isSpectacle = data[u"desktopFile"_s].toString() == qGuiApp->desktopFileName();
}

void VideoPlatformWayland::startRecording(const QUrl &fileUrl, RecordingMode recordingMode, const QVariant &option, bool includePointer)
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
    if (isRecording()) {
        qWarning() << "Warning: Tried to start recording while already recording.";
        return;
    }
    if (!fileUrl.isEmpty() && !fileUrl.isLocalFile()) {
        Q_EMIT recordingFailed(i18nc("@info:shell", "Failed to record: File URL is not a local file"));
        return;
    }

    m_recorder->setActive(false);
    // BUG: https://bugs.kde.org/show_bug.cgi?id=476964
    // CursorMode::Metadata doesn't work.
    Screencasting::CursorMode mode = includePointer ? Screencasting::CursorMode::Embedded : Screencasting::Hidden;
    ScreencastingStream *stream = nullptr;
    switch (recordingMode) {
    case Screen: {
        auto screen = option.value<QScreen *>();
        if (!screen) {
            selectAndRecord(fileUrl, recordingMode, includePointer);
            return;
        }
        Q_ASSERT(screen != nullptr);
        minimizeIfWindowsIntersect(screen->geometry());
        stream = m_screencasting->createOutputStream(screen, mode);
        break;
    }
    case Window: {
        auto window = option.toString();
        if (window.isEmpty()) {
            selectAndRecord(fileUrl, recordingMode, includePointer);
            return;
        }
        Q_ASSERT(!window.isEmpty());
        QDBusMessage message = QDBusMessage::createMethodCall(u"org.kde.KWin"_s,
                                                                u"/KWin"_s,
                                                                u"org.kde.KWin"_s,
                                                                u"getWindowInfo"_s);
        message.setArguments({window});
        const QDBusReply<QVariantMap> reply = QDBusConnection::sessionBus().call(message);
        const auto &data = reply.value();

        QRectF windowRect;
        bool isSpectacle = false;
        if (reply.isValid()) {
            setWindowInfo(data, windowRect, isSpectacle);
            ExportManager::instance()->setWindowTitle(data[u"caption"_s].toString());
        }

        if (!windowRect.isEmpty() && !isSpectacle) {
            minimizeIfWindowsIntersect(windowRect);
        }
        stream = m_screencasting->createWindowStream(window, mode);
        break;
    }
    case Region: {
        auto rect = option.toRectF();
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
        int x = std::ceil<int>(rect.x());
        int y = std::ceil<int>(rect.y());
        // We calculate size using floor(right) - x and floor(bottom) - y so that the rect doesn't
        // shift too much. Ensure size is at least 1x1 to keep the final rect valid.
        int w = std::max<int>(1, std::floor<int>(rect.right()) - x);
        int h = std::max<int>(1, std::floor<int>(rect.bottom()) - y);
        stream = m_screencasting->createRegionStream({x, y, w, h}, scaling, mode);
        break;
    }
    default: break; // This shouldn't happen
    }

    Q_ASSERT(stream);
    connect(stream, &ScreencastingStream::created, this, [this, stream] {
        m_recorder->setNodeId(stream->nodeId());
        if (!m_recorder->output().isEmpty()) {
            m_recorder->setActive(true);
        }
        setRecording(true);
    });
    connect(stream, &ScreencastingStream::failed, this, [this](const QString &error) {
        setRecording(false);
        Q_EMIT recordingFailed(error);
    });
    setupOutput(fileUrl);

    connect(m_recorder.get(), &PipeWireRecord::stateChanged, this, [this] {
        if (m_recorder->state() == PipeWireRecord::Idle && isRecording()) {
            setRecording(false);
            Q_EMIT recordingSaved(QUrl::fromLocalFile(m_recorder->output()));
        }
    });
}

void VideoPlatformWayland::finishRecording()
{
    if (!m_recorder) {
        return;
    }
    m_recorder->setActive(false);
    m_recorder->setNodeId(0);
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

void VideoPlatformWayland::setupOutput(const QUrl &fileUrl)
{
    if (!fileUrl.isValid()) {
        ExportManager::instance()->updateTimestamp();
        const auto format = static_cast<Format>(Settings::preferredVideoFormat());
        auto tempUrl = ExportManager::instance()->tempVideoUrl();
        if (!tempUrl.isLocalFile()) {
            Q_EMIT recordingFailed(i18nc("@info:shell", "Failed to record: Temporary file URL is not a local file (%1)", tempUrl.toString()));
            return;
        }
        if (!mkDirPath(tempUrl)) {
            return;
        }
        m_recorder->setEncoder(encoderForFormat(format));
        m_recorder->setOutput(tempUrl.toLocalFile());
        m_recorder->setActive(m_recorder->nodeId() != 0);
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
        QVariant option;
        if (recordingMode == Screen) {
            QPoint pos = QCursor::pos();
            // BUG: https://bugs.kde.org/show_bug.cgi?id=480599
            // On wayland, you can't always get the cursor position from QCursor::pos().
            // However, using selected window geometry can sometimes select the wrong screen if the
            // window is between screens. We'll need to come up with a better solution someday.
            if (pos.isNull()) {
                pos = {
                    data[u"x"_s].toInt() + data[u"width"_s].toInt() / 2,
                    data[u"y"_s].toInt() + data[u"height"_s].toInt() / 2};
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
            option = QVariant::fromValue(screen);
        } else {
            const auto &windowId = data.value(u"uuid"_s).toString();
            if (windowId.isEmpty()) {
                Q_EMIT recordingFailed(i18nc("@info:shell", "Failed to select window: No window found"));
                return;
            }
            option = windowId;
        }
        startRecording(fileUrl, recordingMode, option, includePointer);
    };
    connect(watcher, &QDBusPendingCallWatcher::finished, this, onFinished);
}

#include "moc_VideoPlatformWayland.cpp"
