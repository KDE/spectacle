/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PlatformNull.h"

#include <QDebug>
#include <QPixmap>

/* -- Null Platform ---------------------------------------------------------------------------- */

PlatformNull::PlatformNull(QObject *parent)
    : Platform(parent)
{
}

Platform::GrabModes PlatformNull::supportedGrabModes() const
{
    return {GrabMode::AllScreens | GrabMode::CurrentScreen | GrabMode::ActiveWindow | GrabMode::WindowUnderCursor | GrabMode::TransientWithParent
            | GrabMode::AllScreensScaled};
}

Platform::ShutterModes PlatformNull::supportedShutterModes() const
{
    return {ShutterMode::Immediate | ShutterMode::OnClick};
}

void PlatformNull::doGrab(ShutterMode theShutterMode, GrabMode theGrabMode, bool theIncludePointer, bool theIncludeDecorations)
{
    Q_UNUSED(theShutterMode)
    Q_UNUSED(theGrabMode)
    Q_UNUSED(theIncludePointer)
    Q_UNUSED(theIncludeDecorations)
    Q_EMIT newScreenshotTaken(QPixmap());
}

VideoPlatformNull::VideoPlatformNull(QObject *parent)
    : VideoPlatform(parent)
{
}

VideoPlatform::RecordingModes VideoPlatformNull::supportedRecordingModes() const
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

QString VideoPlatformNull::extension() const
{
    return QStringLiteral("mp4");
}
