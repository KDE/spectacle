/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SpectacleDBusAdapter.h"
#include "SpectacleCommon.h"
#include "settings.h"

SpectacleDBusAdapter::SpectacleDBusAdapter(SpectacleCore *parent)
    : QDBusAbstractAdaptor(parent)
{
    setAutoRelaySignals(false);
}

inline SpectacleCore *SpectacleDBusAdapter::parent() const
{
    return static_cast<SpectacleCore *>(QObject::parent());
}

void SpectacleDBusAdapter::FullScreen()
{
    parent()->takeNewScreenshot(Spectacle::CaptureMode::AllScreens, 0, Settings::includePointer(), true);
}

void SpectacleDBusAdapter::CurrentScreen()
{
    parent()->takeNewScreenshot(Spectacle::CaptureMode::CurrentScreen, 0, Settings::includePointer(), true);
}

void SpectacleDBusAdapter::ActiveWindow()
{
    parent()->takeNewScreenshot(Spectacle::CaptureMode::ActiveWindow, 0, Settings::includePointer(), Settings::includeDecorations());
}

void SpectacleDBusAdapter::WindowUnderCursor()
{
    parent()->takeNewScreenshot(Spectacle::CaptureMode::WindowUnderCursor, 0, Settings::includePointer(), Settings::includeDecorations());
}

void SpectacleDBusAdapter::RectangularRegion()
{
    parent()->takeNewScreenshot(Spectacle::CaptureMode::RectangularRegion, 0, Settings::includePointer(), false);
}

void SpectacleDBusAdapter::OpenWithoutScreenshot()
{
    parent()->initGuiNoScreenshot();
}
