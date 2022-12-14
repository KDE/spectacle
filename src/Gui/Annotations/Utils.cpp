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
QImage fastPseudoBlur(const QImage &src, int radius, qreal devicePixelRatio)
{
    QImage out = (src.format() != QImage::Format_ARGB32_Premultiplied) ? src.convertToFormat(QImage::Format_ARGB32_Premultiplied) : src;
    QImage out2 = (src.format() != QImage::Format_ARGB32_Premultiplied) ? src.convertToFormat(QImage::Format_ARGB32_Premultiplied) : src;

    QPainter p(&out);
    p.setOpacity(1.0 / radius);
    for (qreal i = -radius; i < 0; ++i) {
        p.drawImage(QPointF(i / devicePixelRatio, 0), src);
        p.drawImage(QPointF(-i / devicePixelRatio, 0), src);
    }
    p.end();

    p.begin(&out2);
    p.setOpacity(1.0 / radius);
    for (qreal i = -radius; i < 0; ++i) {
        p.drawImage(QPointF(0, i / devicePixelRatio), out);
        p.drawImage(QPointF(0, -i / devicePixelRatio), out);
    }
    p.end();

    return out2;
}

QImage shapeShadow(EditAction *action, qreal devicePixelRatio)
{
    if (!action->hasShadow()) {
        return QImage();
    }

    const QColor shadowColor(63, 63, 63, std::min(28, std::max(action->strokeColor().alpha(), action->fillColor().alpha())));

    const QRectF totalRect = action->visualGeometry() + action->shadowMargins();
    QImage shadow(totalRect.size().toSize() * devicePixelRatio, QImage::Format_ARGB32_Premultiplied);
    shadow.fill(Qt::transparent);
    QPainter p(&shadow);
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(shadowColor, action->strokeWidth(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    const QPointF translation = -totalRect.topLeft() + QPointF(shadowOffsetX, shadowOffsetY);

    switch (action->type()) {
    case AnnotationDocument::FreeHand: {
        auto *fha = static_cast<FreeHandAction *>(action);
        auto path = fha->path().translated(translation);
        if (path.isEmpty()) {
            auto start = path.elementAt(0);
            // ensure a single point freehand event is visible
            path.lineTo(start.x + 0.0001, start.y);
        }
        p.drawPath(path);
    }
    case AnnotationDocument::Line:
    case AnnotationDocument::Arrow: {
        auto *la = static_cast<LineAction *>(action);
        const auto &line = la->line().translated(translation);
        p.drawLine(line);
        if (la->type() == AnnotationDocument::Arrow) {
            p.drawPolyline(la->arrowHeadPolygon(line));
        }
    }
    case AnnotationDocument::Rectangle:
    case AnnotationDocument::Ellipse: {
        auto *sa = static_cast<ShapeAction *>(action);
        p.setBrush(shadowColor);
        // p.setPen(sa->strokeWidth() > 0 ? pen : Qt::NoPen);
        // p.setBrush(sa->fillColor());

        const auto rect = sa->geometry().translated(translation);
        switch (sa->type()) {
        case AnnotationDocument::Rectangle:
            p.drawRect(rect);
            break;
        case AnnotationDocument::Ellipse:
            p.drawEllipse(rect);
            break;
        default:
            break;
        }
        break;
    }
    case AnnotationDocument::Text: {
        auto *ta = static_cast<TextAction *>(action);
        p.setPen({63, 63, 63, std::min(28, ta->fontColor().alpha())});
        p.setFont(ta->font());
        QPointF baselineOffset = {0, QFontMetricsF(ta->font()).ascent()};
        p.drawText(ta->startPoint() + baselineOffset + translation, ta->text());
        break;
    }
    case AnnotationDocument::Number: {
        auto *na = static_cast<NumberAction *>(action);
        const auto rect = na->geometry().translated(translation);
        p.setBrush(shadowColor);
        p.setPen(Qt::NoPen);
        p.drawEllipse(rect);
        break;
    }
    default:
        break;
    }

    p.end();
    return fastPseudoBlur(shadow, shadowBlurRadius, devicePixelRatio);
}
