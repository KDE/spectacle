/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "Traits.h"

#include <QImage>

static const int defaultBlurRadius = 4;
static const int shadowBlurRadius = 2;
static const int shadowOffsetX = 2;
static const int shadowOffsetY = 2;

QImage boxBlur(const QImage &src, int radius);
QImage fastPseudoBlur(const QImage &src, int radius, qreal devicePixelRatio = 1);
QImage shapeShadow(const Traits::OptTuple &traits, qreal devicePixelRatio = 1);
