/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "VideoPlatform.h"
#include <KFormat>
#include <chrono>

VideoPlatform::VideoPlatform(QObject *parent)
    : QObject(parent)
{
    using namespace std::chrono_literals;
    m_recordedTimeChanged.setInterval(1s);
    m_recordedTimeChanged.setSingleShot(false);
    connect(&m_recordedTimeChanged, &QTimer::timeout, this, &VideoPlatform::recordedTimeChanged);
}

bool VideoPlatform::isRecording() const
{
    return m_recording;
}

void VideoPlatform::setRecording(bool recording)
{
    // We are asserting because if we start recording on an already
    // started session which is bad usage of the API
    Q_ASSERT(recording != m_recording);
    m_recording = recording;
    Q_EMIT recordingChanged(recording);

    if (recording) {
        m_startedRecording = QDateTime::currentDateTimeUtc();
        m_recordedTimeChanged.start();
    } else {
        m_recordedTimeChanged.stop();
    }
}

QString VideoPlatform::recordedTime() const
{
    auto msecs = m_startedRecording.msecsTo(QDateTime::currentDateTimeUtc());
    return KFormat().formatDuration(msecs);
}
