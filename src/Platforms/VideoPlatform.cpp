/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "VideoPlatform.h"

VideoPlatform::VideoPlatform(QObject *parent)
    : QObject(parent)
{
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
}
