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

void ImagePlatformNull::doGrab(ShutterMode shutterMode, GrabMode grabMode, bool includePointer, bool includeDecorations, bool includeShadow)
{
    Q_UNUSED(shutterMode)
    Q_UNUSED(grabMode)
    Q_UNUSED(includePointer)
    Q_UNUSED(includeDecorations)
    Q_UNUSED(includeShadow)
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

void VideoPlatformNull::startRecording(const QUrl &fileUrl, RecordingMode mode, const QVariant &, bool withPointer)
{
    setRecording(true);
    m_fileUrl = fileUrl;
    qDebug() << "start recording" << mode << "pointer:" << withPointer << "url:" << fileUrl;
}

void VideoPlatformNull::finishRecording()
{
    setRecording(false);
    qDebug() << "finish recording" << m_fileUrl;
    Q_EMIT recordingSaved(m_fileUrl);
}



#include "moc_PlatformNull.cpp"
