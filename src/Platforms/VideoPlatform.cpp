/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "VideoPlatform.h"
#include <QTimerEvent>

using namespace Qt::StringLiterals;

VideoPlatform::VideoPlatform(QObject *parent)
    : QObject(parent)
{
}

bool VideoPlatform::isRecording() const
{
    return m_recordingState == RecordingState::Recording;
}

qint64 VideoPlatform::recordedTime() const
{
    return m_recordedTime;
}

void VideoPlatform::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_basicTimer.timerId()) {
        m_recordedTime = m_elapsedTimer.isValid() ? m_elapsedTimer.elapsed() : 0;
        Q_EMIT recordedTimeChanged();
    }
}

QString VideoPlatform::extensionForFormat(Format format)
{
    switch (format) {
    case WebM_VP9: return "webm"_L1;
    case MP4_H264: return "mp4"_L1;
    case WebP: return "webp"_L1;
    case Gif: return "gif"_L1;
    default: return {};
    }
}

VideoPlatform::Format VideoPlatform::formatForExtension(const QString &extension)
{
    auto lowercaseExtension = extension.toLower();
    if (lowercaseExtension == "webm"_L1) {
        return WebM_VP9;
    } else if (lowercaseExtension == "mp4"_L1) {
        return MP4_H264;
    } else if (lowercaseExtension == "webp"_L1) {
        return WebP;
    } else if (lowercaseExtension == "gif"_L1) {
        return Gif;
    } else {
        return NoFormat;
    }
}

VideoPlatform::Format VideoPlatform::formatForPath(const QString &path)
{
    return formatForExtension(path.mid(path.lastIndexOf(u'.') + 1));
}

VideoPlatform::RecordingState VideoPlatform::recordingState() const
{
    return m_recordingState;
}

void VideoPlatform::setRecordingState(RecordingState state)
{
    if (state == m_recordingState) {
        return;
    }

    m_recordingState = state;
    if (state == RecordingState::NotRecording) {
        m_recordedTime = 0;
        m_elapsedTimer.invalidate();
        m_basicTimer.stop();
    } else if (state == RecordingState::Recording) {
        m_recordedTime = 0;
        m_elapsedTimer.start();
        m_basicTimer.start(1000, Qt::PreciseTimer, this);
    } else {
        if (m_elapsedTimer.isValid()) {
            m_recordedTime = m_elapsedTimer.elapsed();
        }
        m_elapsedTimer.invalidate();
        m_basicTimer.stop();
    }
    m_recordingState = state;
    Q_EMIT recordingStateChanged(state);
    Q_EMIT recordedTimeChanged();
}

#include "moc_VideoPlatform.cpp"
