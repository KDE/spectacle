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
#include <QDebug>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUrl>

using namespace Qt::StringLiterals;

using Format = VideoPlatform::Format;
using Formats = VideoPlatform::Formats;
using Encoder = PipeWireBaseEncodedStream::Encoder;

static std::unique_ptr<QTemporaryDir> s_tempDir;

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
    const auto encoders = m_recorder->suggestedEncoders();
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

VideoPlatformWayland::VideoPlatformWayland(QObject *parent)
    : VideoPlatform(parent)
    , m_screencasting(new Screencasting(this))
    , m_recorder(new PipeWireRecord())
{
    // m_recorder->setMaxFramerate({30, 1});
}

VideoPlatform::RecordingModes VideoPlatformWayland::supportedRecordingModes() const
{
    if (m_screencasting->isAvailable())
        return Screen | Window | Region;
    else
        return {};
}

VideoPlatform::Formats VideoPlatformWayland::supportedFormats() const
{
    Formats formats;
    if (m_screencasting->isAvailable()) {
        const auto encoders = m_recorder->suggestedEncoders();
        for (auto encoder : encoders) {
            formats |= formatForEncoder(encoder);
        }
    }
    return formats;
}

void VideoPlatformWayland::startRecording(const QUrl &fileUrl, RecordingMode recordingMode, const RecordingOption &option, bool includePointer)
{
    if (recordingMode == NoRecordingModes) {
        // We should avoid calling startRecording without a recording mode,
        // but it shouldn't cause a runtime error if we can handle it gracefully.
        Q_EMIT recordingCanceled(u"Recording canceled: No recording mode"_s);
        return;
    }
    if (isRecording()) {
        qWarning() << "Warning: Tried to start recording while already recording.";
        return;
    }
    if (!fileUrl.isEmpty() && !fileUrl.isLocalFile()) {
        Q_EMIT recordingFailed(i18nc("@info:shell", "Failed to record: File URL is not a local file"));
        return;
    }

    Screencasting::CursorMode mode = includePointer ? Screencasting::CursorMode::Metadata : Screencasting::Hidden;
    ScreencastingStream *stream = nullptr;
    switch (recordingMode) {
    case Screen:
        stream = m_screencasting->createOutputStream(std::get<QScreen *>(option), mode);
        break;
    case Window:
        stream = m_screencasting->createWindowStream(std::get<QString>(option), mode);
        break;
    case Region:
        stream = m_screencasting->createRegionStream(std::get<QRect>(option), 1, mode);
        break;
    }

    Q_ASSERT(stream);
    connect(stream, &ScreencastingStream::created, this, [this, stream] {
        m_recorder->setNodeId(stream->nodeId());
        m_recorder->setActive(true);
        setRecording(true);
    });
    connect(stream, &ScreencastingStream::failed, this, [this](const QString &error) {
        setRecording(false);
        Q_EMIT recordingFailed(error);
    });
    setupOutput(fileUrl);

    connect(m_recorder.get(), &PipeWireRecord::stateChanged, this, [this] {
        if (m_recorder->state() == PipeWireRecord::Idle && isRecording()) {
            Q_EMIT recordingSaved(m_recorder->output());
            setRecording(false);
        }
    });
}

void VideoPlatformWayland::finishRecording()
{
    Q_ASSERT(m_recorder);
    m_recorder->setActive(false);
}

void VideoPlatformWayland::setupOutput(const QUrl &fileUrl)
{
    if (!fileUrl.isValid()) {
        // Try to use a temporary location so we can save it properly later like screenshots
        if (!s_tempDir) {
            s_tempDir = std::make_unique<QTemporaryDir>(QDir::tempPath() + u"/Spectacle.XXXXXX"_s);
        }
        const auto format = static_cast<Format>(Settings::preferredVideoFormat());
        auto extension = VideoPlatform::extensionForFormat(format);
        auto output = ExportManager::instance()->suggestedVideoFilename(extension);
        if (s_tempDir->isValid()) {
            auto defaultSaveDirPath = Settings::videoSaveLocation().adjusted(QUrl::StripTrailingSlash).path();
            if (!defaultSaveDirPath.isEmpty()) {
                defaultSaveDirPath += u'/';
            }
            auto reducedPath = output.path();
            reducedPath = reducedPath.right(reducedPath.size() - defaultSaveDirPath.size());
            output.setPath(s_tempDir->path() + u'/' + reducedPath);
        }
        m_recorder->setEncoder(encoderForFormat(format));
        m_recorder->setOutput(output.toLocalFile());
    } else {
        const auto &localFile = fileUrl.toLocalFile();
        auto extension = localFile.mid(localFile.lastIndexOf(u'.') + 1);
        m_recorder->setEncoder(encoderForFormat(formatForExtension(extension)));
        m_recorder->setOutput(fileUrl.toLocalFile());
    }
}

#include "moc_VideoPlatformWayland.cpp"
