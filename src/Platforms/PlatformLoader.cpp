/* This file is part of Spectacle, the KDE screenshot utility
 * Copyright (C) 2019 Boudhayan Gupta <bgupta@kde.org>
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

#include "Config.h"
#include "PlatformLoader.h"

#include "PlatformNull.h"
#include "PlatformKWinWayland.h"

#ifdef XCB_FOUND
#include "PlatformXcb.h"
#endif

#include <KWindowSystem>

PlatformPtr loadPlatform()
{
    qRegisterMetaType<Platform::GrabMode>();
    qRegisterMetaType<Platform::ShutterMode>();

    // We might be using the XCB platform (with Xwayland) in a wayland session,
    // but the X11 grabber won't work in that case. So force the Wayland grabber
    // in Wayland sessions.
    if (KWindowSystem::isPlatformWayland() || qstrcmp(qgetenv("XDG_SESSION_TYPE"), "wayland") == 0) {
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
