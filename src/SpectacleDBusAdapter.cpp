/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SpectacleDBusAdapter.h"
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

void SpectacleDBusAdapter::FullScreen(int includeMousePointer)
{
    parent()->takeNewScreenshot(CaptureModeModel::AllScreens, 0, (includeMousePointer == -1) ? Settings::includePointer() : includeMousePointer, true);
}

void SpectacleDBusAdapter::CurrentScreen(int includeMousePointer)
{
    parent()->takeNewScreenshot(CaptureModeModel::CurrentScreen, 0, (includeMousePointer == -1) ? Settings::includePointer() : includeMousePointer, true);
}

void SpectacleDBusAdapter::ActiveWindow(int includeWindowDecorations, int includeMousePointer)
{
    parent()->takeNewScreenshot(CaptureModeModel::ActiveWindow,
                                0,
                                (includeMousePointer == -1) ? Settings::includePointer() : includeMousePointer,
                                includeWindowDecorations == -1 ? Settings::includeDecorations() : includeWindowDecorations);
}

void SpectacleDBusAdapter::WindowUnderCursor(int includeWindowDecorations, int includeMousePointer)
{
    parent()->takeNewScreenshot(CaptureModeModel::WindowUnderCursor,
                                0,
                                (includeMousePointer == -1) ? Settings::includePointer() : includeMousePointer,
                                includeWindowDecorations == -1 ? Settings::includeDecorations() : includeWindowDecorations);
}

void SpectacleDBusAdapter::RectangularRegion(int includeMousePointer)
{
    parent()->takeNewScreenshot(CaptureModeModel::RectangularRegion,
                                0,
                                (includeMousePointer == -1) ? Settings::includePointer() : includeMousePointer,
                                false);
}

void SpectacleDBusAdapter::OpenWithoutScreenshot()
{
    parent()->initGuiNoScreenshot();
}
