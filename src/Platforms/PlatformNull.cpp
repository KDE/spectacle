/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PlatformNull.h"

#include <QPixmap>

/* -- Null Platform ---------------------------------------------------------------------------- */

PlatformNull::PlatformNull(QObject *parent) :
    Platform(parent)
{}

QString PlatformNull::platformName() const
{
    return QStringLiteral("Null");
}

Platform::GrabModes PlatformNull::supportedGrabModes() const
{
    return { GrabMode::AllScreens | GrabMode::CurrentScreen | GrabMode::ActiveWindow | GrabMode::WindowUnderCursor | GrabMode::TransientWithParent | GrabMode::AllScreensScaled };
}

Platform::ShutterModes PlatformNull::supportedShutterModes() const
{
    return { ShutterMode::Immediate | ShutterMode::OnClick };
}

void PlatformNull::doGrab(ShutterMode theShutterMode, GrabMode theGrabMode, bool theIncludePointer, bool theIncludeDecorations)
{
    Q_UNUSED(theShutterMode)
    Q_UNUSED(theGrabMode)
    Q_UNUSED(theIncludePointer)
    Q_UNUSED(theIncludeDecorations)
    emit newScreenshotTaken(QPixmap());
}
