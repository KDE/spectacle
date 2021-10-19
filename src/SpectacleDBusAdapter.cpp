/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
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

void SpectacleDBusAdapter::OpenWithoutScreenshot()
{
    parent()->initGuiNoScreenshot();
}
