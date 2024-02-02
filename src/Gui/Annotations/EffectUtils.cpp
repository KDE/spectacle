/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "EffectUtils.h"

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
QImage fastPseudoBlur(const QImage &src, int radius, qreal devicePixelRatio)
{
    QImage out = (src.format() != QImage::Format_ARGB32_Premultiplied) ? src.convertToFormat(QImage::Format_ARGB32_Premultiplied) : src;
    QImage out2 = (src.format() != QImage::Format_ARGB32_Premultiplied) ? src.convertToFormat(QImage::Format_ARGB32_Premultiplied) : src;

    QPainter p(&out);
    p.setOpacity(1.0 / radius);
    for (int i = -radius; i < 0; ++i) {
        p.drawImage(QPointF(i / devicePixelRatio, 0), src);
        p.drawImage(QPointF(-i / devicePixelRatio, 0), src);
    }
    p.end();

    p.begin(&out2);
    p.setOpacity(1.0 / radius);
    for (int i = -radius; i < 0; ++i) {
        p.drawImage(QPointF(0, i / devicePixelRatio), out);
        p.drawImage(QPointF(0, -i / devicePixelRatio), out);
    }
    p.end();

    return out2;
}

QImage shapeShadow(const Traits::OptTuple &traits, qreal devicePixelRatio)
{
    auto &geometryTrait = std::get<Traits::Geometry::Opt>(traits);
    auto &shadowTrait = std::get<Traits::Shadow::Opt>(traits);
    if (!(geometryTrait && Traits::isValidTrait(geometryTrait.value())) || !shadowTrait) {
        return QImage();
    }

    const QRectF &totalRect = geometryTrait->visualRect;
    QImage shadow(totalRect.size().toSize() * devicePixelRatio, QImage::Format_ARGB32_Premultiplied);
    shadow.fill(Qt::transparent);
    QPainter p(&shadow);
    p.setRenderHint(QPainter::Antialiasing);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::NoBrush);
    p.scale(devicePixelRatio, devicePixelRatio);
    p.translate(-totalRect.topLeft() + QPointF{Traits::Shadow::margins.left(), Traits::Shadow::margins.top()});

    auto &fillTrait = std::get<Traits::Fill::Opt>(traits);
    auto &strokeTrait = std::get<Traits::Stroke::Opt>(traits);
    auto *fillBrush = fillTrait && Traits::isValidTrait(fillTrait.value()) //
        ? std::get_if<Traits::Fill::Brush>(&fillTrait.value())
        : nullptr;
    bool hasStroke = strokeTrait && Traits::isValidTrait(strokeTrait.value());
    // No need to draw fill and stroke separately if they're both opaque
    if (fillBrush && hasStroke && fillBrush->isOpaque() && strokeTrait->pen.brush().isOpaque()) {
        p.setBrush(QColor(0, 0, 0, 28));
        p.drawPath((strokeTrait->path | geometryTrait->path).simplified());
    } else {
        if (fillBrush) {
            p.setBrush(QColor(0, 0, 0, std::ceil(28 * fillBrush->color().alphaF())));
            p.drawPath(geometryTrait->path);
        }
        if (strokeTrait) {
            p.setBrush(QColor(0, 0, 0, std::ceil(28 * strokeTrait->pen.color().alphaF())));
            p.drawPath(strokeTrait->path);
        }
    }

    auto &textTrait = std::get<Traits::Text::Opt>(traits);
    // No need to paint text/number shadow if fill is opaque.
    if ((!fillTrait || (fillBrush && !fillBrush->isOpaque())) && textTrait) {
        p.setFont(textTrait->font);
        p.setBrush(Qt::NoBrush);
        p.setPen(QColor(0, 0, 0, std::ceil(28 * textTrait->brush.color().alphaF())));
        p.drawText(geometryTrait->path.boundingRect(), textTrait->textFlags(), textTrait->text());
    }

    p.end();
    return fastPseudoBlur(shadow, Traits::Shadow::blurRadius, devicePixelRatio);
}
