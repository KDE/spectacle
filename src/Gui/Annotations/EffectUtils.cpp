/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "EffectUtils.h"
// #include "QtCV.h"

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
    auto &shadowTrait = std::get<Traits::Shadow::Opt>(traits);
    if (!Traits::isVisible(traits) || !shadowTrait) {
        return QImage();
    }

    auto &geometryTrait = std::get<Traits::Geometry::Opt>(traits);
    auto &visualTrait = std::get<Traits::Visual::Opt>(traits);
    QImage shadow(visualTrait->rect.size().toSize() * devicePixelRatio, QImage::Format_RGBA8888_Premultiplied);
    shadow.fill(Qt::transparent);
    QPainter p(&shadow);
    p.setRenderHint(QPainter::Antialiasing);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::NoBrush);
    p.scale(devicePixelRatio, devicePixelRatio);
    p.translate(-visualTrait->rect.topLeft() //
                + QPointF{Traits::Shadow::xOffset, Traits::Shadow::yOffset});

    static constexpr auto alpha = 0.5;
    // Convenience var so we don't keep multiplying alpha by 255.
    static constexpr uint8_t alpha8bit = alpha * 255;

    auto &fillTrait = std::get<Traits::Fill::Opt>(traits);
    auto &strokeTrait = std::get<Traits::Stroke::Opt>(traits);
    auto *fillBrush = fillTrait && Traits::isValidTrait(fillTrait.value()) //
        ? std::get_if<Traits::Fill::Brush>(&fillTrait.value())
        : nullptr;
    bool hasStroke = strokeTrait && Traits::isValidTrait(strokeTrait.value());
    // No need to draw fill and stroke separately if they're both opaque
    if (fillBrush && hasStroke && fillBrush->isOpaque() && strokeTrait->pen.brush().isOpaque()) {
        p.setBrush(QColor(0, 0, 0, alpha8bit));
        p.drawPath((strokeTrait->path | geometryTrait->path).simplified());
    } else {
        if (fillBrush) {
            p.setBrush(QColor(0, 0, 0, std::ceil(alpha8bit * fillBrush->color().alphaF())));
            p.drawPath(geometryTrait->path);
        }
        if (strokeTrait) {
            p.setBrush(QColor(0, 0, 0, std::ceil(alpha8bit * strokeTrait->pen.color().alphaF())));
            p.drawPath(strokeTrait->path);
        }
    }

    auto &textTrait = std::get<Traits::Text::Opt>(traits);
    // No need to paint text/number shadow if fill is opaque.
    if ((!fillTrait || (fillBrush && !fillBrush->isOpaque())) && textTrait) {
        p.setFont(textTrait->font);
        p.setBrush(Qt::NoBrush);
        p.setPen(Qt::black);
        // Color emojis don't get semi-transparent shadows with a semi-transparent pen.
        // setOpacity disables sub-pixel text antialiasing, but we don't need sub-pixel AA here.
        p.setOpacity(alpha * textTrait->brush.color().alphaF());
        p.drawText(geometryTrait->path.boundingRect(), textTrait->textFlags(), textTrait->text());
    }
    p.end();
    // auto mat = QtCV::qImageToMat(shadow);
    // const qreal sigma = Traits::Shadow::radius * devicePixelRatio;
    // const int ksize = QtCV::sigmaToKSize(sigma);
    // Do this before converting to Alpha8 because stackBlur gets distorted with Alpha8.
    // QtCV::stackOrGaussianBlurCompatibility(mat, mat, {ksize, ksize}, sigma, sigma);
    // We only want black shadows with opacity, so we only need black and 8 bits of alpha.
    // If we don't do this, color emojis won't have black semi-transparent shadows.
    shadow.convertTo(QImage::Format_Alpha8);
    return shadow;
}
