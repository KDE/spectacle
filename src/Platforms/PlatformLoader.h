/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "Platform.h"
#include <memory>

using PlatformPtr = std::unique_ptr<Platform>;
PlatformPtr loadPlatform();
