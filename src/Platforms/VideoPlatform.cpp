/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "VideoPlatform.h"
#include <KFormat>
#include <QTimerEvent>

VideoPlatform::VideoPlatform(QObject *parent)
    : QObject(parent)
{
}

bool VideoPlatform::isRecording() const
{
    return m_basicTimer.isActive();
}

void VideoPlatform::setRecording(bool recording)
{
    // We are asserting because if we start recording on an already
    // started session which is bad usage of the API
    Q_ASSERT(recording != m_basicTimer.isActive());

    if (recording) {
        m_elapsedTimer.start();
        m_basicTimer.start(1000, Qt::PreciseTimer, this);
    } else {
        m_elapsedTimer.invalidate();
        m_basicTimer.stop();
    }
    Q_EMIT recordingChanged(recording);
    Q_EMIT recordedTimeChanged();
}

QString VideoPlatform::recordedTime() const
{
    return KFormat().formatDuration(m_elapsedTimer.isValid() ? m_elapsedTimer.elapsed() : 0);
}

void VideoPlatform::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_basicTimer.timerId()) {
        Q_EMIT recordedTimeChanged();
    }
}
