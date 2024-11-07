/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QtGlobal>

#include "spectaclecore_export.h"

class SPECTACLECORE_EXPORT ScreenShotEffect
{
public:
    static bool isLoaded();
    static quint32 version();

    enum {
        NullVersion = 0
    };

private:
    ScreenShotEffect() = delete;
    ~ScreenShotEffect() = delete;
    ScreenShotEffect(const ScreenShotEffect &) = delete;
    ScreenShotEffect(ScreenShotEffect &&) = delete;
    ScreenShotEffect &operator=(const ScreenShotEffect &) = delete;
    ScreenShotEffect &operator=(ScreenShotEffect &&) = delete;
};
