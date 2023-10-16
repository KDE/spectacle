/*
    SPDX-FileCopyrightText: 2023 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "VideoPlatformWayland.h"
#include "Platforms/VideoPlatform.h"
#include "screencasting.h"
#include "settings.h"
#include <QDebug>
#include <QStandardPaths>
#include <QUrl>

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

void VideoPlatformWayland::startRecording
(const QString &path, RecordingMode recordingMode, const RecordingOption &option, bool includePointer)
{
    if (isRecording()) {
        qWarning() << "Warning: Tried to start recording while already recording.";
        return;
    }

    auto format = static_cast<Format>(Settings::preferredVideoFormat());
    m_recorder->setEncoder(encoderForFormat(format));
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
    m_recorder->setOutput(path);

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

#include "moc_VideoPlatformWayland.cpp"
