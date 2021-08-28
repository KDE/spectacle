/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

namespace Spectacle
{
enum CaptureMode {
    InvalidChoice = -1,
    AllScreens = 0,
    CurrentScreen = 1,
    ActiveWindow = 2,
    WindowUnderCursor = 3,
    TransientWithParent = 4,
    RectangularRegion = 5,
    AllScreensScaled = 6,
};
}
