/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PlatformLoader.h"
#include "Config.h"

#include "PlatformKWinWayland.h"
#include "PlatformKWinWayland2.h"
#include "PlatformNull.h"
#include "VideoPlatformWayland.h"

#ifdef XCB_FOUND
#include "PlatformXcb.h"
#endif

#include <KWindowSystem>

PlatformPtr loadPlatform()
{
    // We might be using the XCB platform (with Xwayland) in a wayland session,
    // but the X11 grabber won't work in that case. So force the Wayland grabber
    // in Wayland sessions.
    if (KWindowSystem::isPlatformWayland() || qstrcmp(qgetenv("XDG_SESSION_TYPE").constData(), "wayland") == 0) {
        std::unique_ptr<Platform> platform = PlatformKWinWayland2::create();
        if (platform) {
            return platform;
        }
        return std::make_unique<PlatformKWinWayland>();
    }

    // Try checking if we're running under X11 now
#ifdef XCB_FOUND
    if (KWindowSystem::isPlatformX11()) {
        return std::make_unique<PlatformXcb>();
    }
#endif

    // If nothing else worked, return the null platform
    return std::make_unique<PlatformNull>();
}

VideoPlatformPtr loadVideoPlatform()
{
    if (KWindowSystem::isPlatformWayland()) {
        return std::make_unique<VideoPlatformWayland>();
    }
    return std::make_unique<VideoPlatformNull>();
}
