/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PlatformLoader.h"
#include "Config.h"
#include "ScreenShotEffect.h"

#include "PlatformKWin.h"
#include "PlatformNull.h"
#include "VideoPlatformWayland.h"

#ifdef XCB_FOUND
#include "PlatformXcb.h"
#endif

#include <KWindowSystem>

PlatformPtr loadPlatform()
{
    PlatformPtr platform;
    if (ScreenShotEffect::isLoaded()) {
        platform = PlatformKWin::create();
    }
#ifdef XCB_FOUND
    else if (KWindowSystem::isPlatformX11() && qstrcmp(qgetenv("XDG_SESSION_TYPE").constData(), "wayland") != 0) {
        platform = std::make_unique<PlatformXcb>();
    }
#endif

    // If nothing else worked, return the null platform
    if (!platform) {
        platform = std::make_unique<PlatformNull>();
    }

    return platform;
}

VideoPlatformPtr loadVideoPlatform()
{
    if (KWindowSystem::isPlatformWayland()) {
        return std::make_unique<VideoPlatformWayland>();
    }
    return std::make_unique<VideoPlatformNull>();
}
