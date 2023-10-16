/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "ImagePlatform.h"
#include "VideoPlatform.h"
#include <memory>

using ImagePlatformPtr = std::unique_ptr<ImagePlatform>;
ImagePlatformPtr loadImagePlatform();

using VideoPlatformPtr = std::unique_ptr<VideoPlatform>;
VideoPlatformPtr loadVideoPlatform();
