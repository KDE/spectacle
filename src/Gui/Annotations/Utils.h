/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QImage>

QImage boxBlur(const QImage &src, int radius);
QImage fastPseudoBlur(const QImage &src, int radius);
void boxBlurAlpha(QImage &image, int radius);
