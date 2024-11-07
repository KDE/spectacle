/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "VideoPlatformNull.h"

#include <QDebug>
#include <QPixmap>

/* -- Null Platform ---------------------------------------------------------------------------- */

VideoPlatformNull::VideoPlatformNull(const QString &unavailableMessage, QObject *parent)
    : VideoPlatform(parent)
    , m_unavailableMessage(unavailableMessage)
{
}

VideoPlatform::RecordingModes VideoPlatformNull::supportedRecordingModes() const
{
    return {};
}

VideoPlatform::Formats VideoPlatformNull::supportedFormats() const
{
    return {};
}

void VideoPlatformNull::startRecording(const QUrl &fileUrl, RecordingMode mode, const QVariantMap &options, bool withPointer)
{
    Q_UNUSED(fileUrl)
    Q_UNUSED(mode)
    Q_UNUSED(options)
    Q_UNUSED(withPointer)
    Q_EMIT recordingFailed(m_unavailableMessage);
}

void VideoPlatformNull::finishRecording()
{
    setRecording(false);
}

#include "moc_VideoPlatformNull.cpp"
