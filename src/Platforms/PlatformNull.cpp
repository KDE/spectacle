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
    Q_EMIT newScreenshotFailed();
}

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
}

#include "moc_PlatformNull.cpp"
