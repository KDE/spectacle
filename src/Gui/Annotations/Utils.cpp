/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "Utils.h"

#include <QDebug>
#include <QPainter>
#include <QtMath>
// TODO: switch to this slightly more high quality one (or even gaussian blur) when live editing is over?
QImage boxBlur(const QImage &src, int radius)
{
    QImage blurX = (src.format() != QImage::Format_ARGB32_Premultiplied) ? src.convertToFormat(QImage::Format_ARGB32_Premultiplied) : src;

    int count = 0;
    int r = 0;
    int g = 0;
    int b = 0;
    int a = 0;
    QRgb rgb;
    for (int i = 0; i < blurX.height(); ++i) {
        for (int j = 0; j < blurX.width(); ++j) {
            count = 0;
            r = 0;
            g = 0;
            b = 0;
            a = 0;
            for (int k = qMax(0, j - radius); k < qMin(blurX.width(), j + radius); ++k) {
                rgb = src.pixel(k, i);
                r += qRed(rgb);
                g += qGreen(rgb);
                b += qBlue(rgb);
                a += qAlpha(rgb);
                ++count;
            }
            blurX.setPixel(j, i, qRgba(r / count, g / count, b / count, a / count));
        }
    }
    // QImage blurY = blurX;
    for (int i = 0; i < blurX.width(); ++i) {
        for (int j = 0; j < blurX.height(); ++j) {
            count = 0;
            r = 0;
            g = 0;
            b = 0;
            a = 0;
            for (int k = qMax(0, j - radius); k < qMin(blurX.height(), j + radius); ++k) {
                rgb = blurX.pixel(i, k);
                r += qRed(rgb);
                g += qGreen(rgb);
                b += qBlue(rgb);
                a += qAlpha(rgb);
                ++count;
            }
            blurX.setPixel(i, j, qRgba(r / count, g / count, b / count, a / count));
        }
    }
    return blurX;
}

/**
 * This thing kinda looks like a box blur, but is limited at painting over with qpainter opacity rather than proper color mixing
 * It looks a bit worse than proper box blur but is much faster
 */
QImage fastPseudoBlur(const QImage &src, int radius)
{
    QImage out = (src.format() != QImage::Format_ARGB32_Premultiplied) ? src.convertToFormat(QImage::Format_ARGB32_Premultiplied) : src;

    QPainter p(&out);
    p.setOpacity(1.0 / radius);
    for (int i = 0; i < out.width(); ++i) {
        for (int k = qMax(0, i - radius); k < qMin(out.width(), i + radius); ++k) {
            p.drawImage(QRectF(i, 0, 1, src.height()), src, QRectF(k, 0, 1, src.height()));
        }
    }
    for (int i = 0; i < out.height(); ++i) {
        for (int k = qMax(0, i - radius); k < qMin(out.width(), i + radius); ++k) {
            p.drawImage(QRectF(0, i, src.width(), 1), out, QRectF(0, k, src.width(), 1));
        }
    }

    p.end();
    return out;
}
