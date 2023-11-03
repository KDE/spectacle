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

qint64 VideoPlatform::recordedTime() const
{
    return m_elapsedTimer.isValid() ? m_elapsedTimer.elapsed() : 0;
}

void VideoPlatform::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_basicTimer.timerId()) {
        Q_EMIT recordedTimeChanged();
    }
}

QString VideoPlatform::extensionForFormat(Format format)
{
    switch (format) {
    case WebM_VP9: return "webm"_L1;
    case MP4_H264: return "mp4"_L1;
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
    } else {
        return NoFormat;
    }
}

#include "moc_VideoPlatform.cpp"
