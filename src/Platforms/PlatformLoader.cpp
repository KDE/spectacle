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

#include <KLocalizedString>
#include <KWindowSystem>

#include <QDebug>

ImagePlatformPtr getForcedImagePlatform()
{
    // This environment variable is only for testing purposes.
    auto platformName = qgetenv("SPECTACLE_IMAGE_PLATFORM");
    if (platformName.isEmpty()) {
        return nullptr;
    }

    if (platformName == ImagePlatformKWin::staticMetaObject.className()) {
        return std::make_unique<ImagePlatformKWin>();
    } else if (platformName == ImagePlatformXcb::staticMetaObject.className()) {
        return std::make_unique<ImagePlatformXcb>();
    } else if (platformName == ImagePlatformNull::staticMetaObject.className()) {
        return std::make_unique<ImagePlatformNull>();
    } else if (!platformName.isEmpty()) {
        qWarning() << "SPECTACLE_IMAGE_PLATFORM:" << platformName << "is invalid";
    }

    return nullptr;
}

ImagePlatformPtr loadImagePlatform()
{
    if (auto platform = getForcedImagePlatform()) {
        return platform;
    }

    // Check XDG_SESSION_TYPE because Spectacle might be using the XCB platform via XWayland
    const bool isReallyX11 = KWindowSystem::isPlatformX11() && qstrcmp(qgetenv("XDG_SESSION_TYPE").constData(), "wayland") != 0;
    // Before KWin 5.27.8, there was an infinite loop in KWin on X11 when doing rectangle captures.
    // Spectacle uses CaptureScreen DBus calls to KWin for rectangle captures.
    if (ScreenShotEffect::isLoaded() && ScreenShotEffect::version() != ScreenShotEffect::NullVersion
        && (!isReallyX11 || PlasmaVersion::get() >= PlasmaVersion::check(5, 27, 8))) {
        return std::make_unique<ImagePlatformKWin>();
    }
#ifdef XCB_FOUND
    else if (isReallyX11) {
        return std::make_unique<ImagePlatformXcb>();
    }
#endif
    // If nothing else worked, return the null platform
    return std::make_unique<ImagePlatformNull>();
}

VideoPlatformPtr getForcedVideoPlatform()
{
    // This environment variable is only for testing purposes.
    auto platformName = qgetenv("SPECTACLE_VIDEO_PLATFORM");
    if (platformName.isEmpty()) {
        return nullptr;
    }

    if (platformName == VideoPlatformWayland::staticMetaObject.className()) {
        return std::make_unique<VideoPlatformWayland>();
    } else if (platformName == VideoPlatformNull::staticMetaObject.className()) {
        return std::make_unique<VideoPlatformNull>();
    } else if (!platformName.isEmpty()) {
        qWarning() << "SPECTACLE_VIDEO_PLATFORM:" << platformName << "is invalid";
    }

    return nullptr;
}

VideoPlatformPtr loadVideoPlatform()
{
    if (auto platform = getForcedVideoPlatform()) {
        return platform;
    }
    if (KWindowSystem::isPlatformWayland()) {
        return std::make_unique<VideoPlatformWayland>();
    }
    if (KWindowSystem::isPlatformX11()) {
        return std::make_unique<VideoPlatformNull>(i18nc("@info", "Screen recording is not available on X11."));
    }
    return std::make_unique<VideoPlatformNull>();
}
