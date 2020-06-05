/* This file is part of Spectacle, the KDE screenshot utility
 * Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
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

#include "SpectacleDBusAdapter.h"
#include "SpectacleCommon.h"

SpectacleDBusAdapter::SpectacleDBusAdapter(SpectacleCore *parent)
    : QDBusAbstractAdaptor(parent)
{
    setAutoRelaySignals(false);
}

inline SpectacleCore *SpectacleDBusAdapter::parent() const
{
    return static_cast<SpectacleCore *>(QObject::parent());
}

void SpectacleDBusAdapter::FullScreen(bool includeMousePointer)
{
    parent()->takeNewScreenshot(Spectacle::CaptureMode::AllScreens, 0, includeMousePointer, true);
}

void SpectacleDBusAdapter::CurrentScreen(bool includeMousePointer)
{
    parent()->takeNewScreenshot(Spectacle::CaptureMode::CurrentScreen, 0, includeMousePointer, true);
}

void SpectacleDBusAdapter::ActiveWindow(bool includeWindowDecorations, bool includeMousePointer)
{
    parent()->takeNewScreenshot(Spectacle::CaptureMode::ActiveWindow, 0, includeMousePointer, includeWindowDecorations);
}

void SpectacleDBusAdapter::WindowUnderCursor(bool includeWindowDecorations, bool includeMousePointer)
{
    parent()->takeNewScreenshot(Spectacle::CaptureMode::WindowUnderCursor, 0, includeMousePointer, includeWindowDecorations);
}

void SpectacleDBusAdapter::RectangularRegion(bool includeMousePointer)
{
    parent()->takeNewScreenshot(Spectacle::CaptureMode::RectangularRegion, 0, includeMousePointer, false);
}
