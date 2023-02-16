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
    Q_ASSERT(!m_recorder);
    m_recorder.reset(new PipeWireRecord());

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
            m_recorder.reset();
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
    return PipeWireRecord::extension();
}
