/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QScreen>
#include <QImage>

struct ScreenImage
{
    QScreen *screen;
    QImage image;
    qreal devicePixelRatio = 1; // ensure a valid default value
};
