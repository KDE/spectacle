/*
 *  Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "SpectacleDBusAdapter.h"

SpectacleDBusAdapter::SpectacleDBusAdapter(KSCore *parent)
    : QDBusAbstractAdaptor(parent)
{
    setAutoRelaySignals(false);
}

SpectacleDBusAdapter::~SpectacleDBusAdapter()
{}

inline KSCore * SpectacleDBusAdapter::parent() const
{
    return static_cast<KSCore *>(QObject::parent());
}

Q_NOREPLY void SpectacleDBusAdapter::StartAgent()
{
    parent()->dbusStartAgent();
}

Q_NOREPLY void SpectacleDBusAdapter::FullScreen(bool includeMousePointer)
{
    parent()->takeNewScreenshot(ImageGrabber::FullScreen, 0, includeMousePointer, true);
}

Q_NOREPLY void SpectacleDBusAdapter::CurrentScreen(bool includeMousePointer)
{
    parent()->takeNewScreenshot(ImageGrabber::CurrentScreen, 0, includeMousePointer, true);
}

Q_NOREPLY void SpectacleDBusAdapter::ActiveWindow(bool includeWindowDecorations, bool includeMousePointer)
{
    parent()->takeNewScreenshot(ImageGrabber::ActiveWindow, 0, includeMousePointer, includeWindowDecorations);
}

Q_NOREPLY void SpectacleDBusAdapter::WindowUnderCursor(bool includeWindowDecorations, bool includeMousePointer)
{
    parent()->takeNewScreenshot(ImageGrabber::WindowUnderCursor, 0, includeMousePointer, includeWindowDecorations);
}

