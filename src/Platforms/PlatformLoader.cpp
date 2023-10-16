/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PlatformLoader.h"
#include "Config.h"
#include "PlasmaVersion.h"
#include "ScreenShotEffect.h"

#include "ImagePlatformKWin.h"
#include "PlatformNull.h"
#include "VideoPlatformWayland.h"

#ifdef XCB_FOUND
#include "ImagePlatformXcb.h"
#endif

#include <KWindowSystem>

ImagePlatformPtr loadImagePlatform()
{
    ImagePlatformPtr platform;
    // Check XDG_SESSION_TYPE because Spectacle might be using the XCB platform via XWayland
    const bool isReallyX11 = KWindowSystem::isPlatformX11() && qstrcmp(qgetenv("XDG_SESSION_TYPE").constData(), "wayland") != 0;
    // Before KWin 5.27.8, there was an infinite loop in KWin on X11 when doing rectangle captures.
    // Spectacle uses CaptureScreen DBus calls to KWin for rectangle captures.
    if (ScreenShotEffect::isLoaded() && (!isReallyX11 || PlasmaVersion::get() >= PlasmaVersion::check(5, 27, 8))) {
        platform = ImagePlatformKWin::create();
    }
#ifdef XCB_FOUND
    else if (isReallyX11) {
        platform = std::make_unique<ImagePlatformXcb>();
    }
#endif

    // If nothing else worked, return the null platform
    if (!platform) {
        platform = std::make_unique<ImagePlatformNull>();
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
