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

void PlatformNull::doGrab(ShutterMode shutterMode, GrabMode grabMode, bool includePointer, bool includeDecorations)
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

void VideoPlatformNull::setExtension(const QString &)
{
}

QString VideoPlatformNull::extension() const
{
    return QStringLiteral("mp4");
}

QStringList VideoPlatformNull::suggestedExtensions() const
{
    return {QStringLiteral("mp4")};
}
