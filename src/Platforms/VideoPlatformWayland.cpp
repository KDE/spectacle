/*
    SPDX-FileCopyrightText: 2023 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "VideoPlatformWayland.h"
#include "screencasting.h"
#include <QDebug>
#include <QStandardPaths>
#include <QUrl>

VideoPlatformWayland::VideoPlatformWayland(QObject *parent)
    : VideoPlatform(parent)
    , m_screencasting(new Screencasting(this))
    , m_recorder(new PipeWireRecord())
{
}

VideoPlatform::RecordingModes VideoPlatformWayland::supportedRecordingModes() const
{
    if (m_screencasting->isAvailable())
        return Screen | Window | Region;
    else
        return {};
}

void VideoPlatformWayland::startRecording(const QString &path, RecordingMode recordingMode, const RecordingOption &option, bool includePointer)
{
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

QString VideoPlatformWayland::extension() const
{
    return m_recorder->currentExtension();
}

QStringList VideoPlatformWayland::suggestedExtensions() const
{
    QStringList extensions;

    for (const PipeWireBaseEncodedStream::Encoder enc : m_recorder->suggestedEncoders()) {
        if (enc == PipeWireBaseEncodedStream::VP8) {
            extensions.append(QStringLiteral("webm"));
        } else if (enc == PipeWireBaseEncodedStream::H264Baseline || enc == PipeWireBaseEncodedStream::H264Main) {
            extensions.append(QStringLiteral("mp4"));
        }
    }
    return extensions;

}
void VideoPlatformWayland::setExtension(const QString &extension)
{
    if (extension == QStringLiteral("webm")) {
        m_recorder->setEncoder(PipeWireBaseEncodedStream::VP8);
    } else if (extension == QStringLiteral("mp4")) {
        m_recorder->setEncoder(PipeWireBaseEncodedStream::H264Main);
    } else {
        qWarning() << "Unsupported extension" << extension;
    }
}

#include "moc_VideoPlatformWayland.cpp"
