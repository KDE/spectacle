/* This file is part of Spectacle, the KDE screenshot utility
 * Copyright (C) 2019 Boudhayan Gupta <bgupta@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
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
    return { GrabMode::AllScreens | GrabMode::CurrentScreen | GrabMode::ActiveWindow | GrabMode::WindowUnderCursor | GrabMode::TransientWithParent };
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
