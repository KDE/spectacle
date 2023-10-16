/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PlatformNull.h"

#include <QDebug>
#include <QPixmap>

/* -- Null Platform ---------------------------------------------------------------------------- */

ImagePlatformNull::ImagePlatformNull(QObject *parent)
    : ImagePlatform(parent)
{
}

ImagePlatform::GrabModes ImagePlatformNull::supportedGrabModes() const
{
    return {GrabMode::AllScreens | GrabMode::CurrentScreen | GrabMode::ActiveWindow | GrabMode::WindowUnderCursor | GrabMode::TransientWithParent
            | GrabMode::AllScreensScaled};
}

ImagePlatform::ShutterModes ImagePlatformNull::supportedShutterModes() const
{
    return {ShutterMode::Immediate | ShutterMode::OnClick};
}

void ImagePlatformNull::doGrab(ShutterMode shutterMode, GrabMode grabMode, bool includePointer, bool includeDecorations)
{
    Q_UNUSED(shutterMode)
    Q_UNUSED(grabMode)
    Q_UNUSED(includePointer)
    Q_UNUSED(includeDecorations)
    Q_EMIT newScreenshotTaken();
}

VideoPlatformNull::VideoPlatformNull(QObject *parent)
    : VideoPlatform(parent)
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

void VideoPlatformNull::startRecording(const QString &path, RecordingMode mode, const RecordingOption &, bool withPointer)
{
    setRecording(true);
    m_path = path;
    qDebug() << "start recording" << mode << "pointer:" << withPointer << path;
}

void VideoPlatformNull::finishRecording()
{
    setRecording(false);
    qDebug() << "finish recording" << m_path;
    Q_EMIT recordingSaved(m_path);
}



#include "moc_PlatformNull.cpp"
