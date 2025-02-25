/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "Geometry.h"

#include <KWindowSystem>

#include <QDebug>
#include <QGuiApplication>
#include <QPainterPath>
#include <QScreen>
#include <QWindow>

#include <cmath>

Geometry::Geometry(QObject *parent)
    : QObject(parent)
{}

qreal Geometry::dpx(qreal dpr)
{
    return 1 / dpr;
}

qreal Geometry::dprRound(qreal value, qreal dpr)
{
    return std::round(value * dpr) / dpr;
}

QPointF Geometry::dprRound(QPointF point, qreal dpr)
{
    return {std::round(point.x() * dpr) / dpr, std::round(point.y() * dpr) / dpr};
}

qreal Geometry::dprCeil(qreal value, qreal dpr)
{
    return std::ceil(value * dpr) / dpr;
}

qreal Geometry::dprFloor(qreal value, qreal dpr)
{
    return std::floor(value * dpr) / dpr;
}

qreal Geometry::mapFromPlatformValue(qreal value, qreal dpr)
{
    if (KWindowSystem::isPlatformX11()) {
        return value / dpr;
    }
    return value;
}

QPointF Geometry::mapFromPlatformPoint(const QPointF &point, qreal dpr)
{
    if (KWindowSystem::isPlatformX11()) {
        return point / dpr;
    }
    return point;
}

QRectF Geometry::mapFromPlatformRect(const QRectF &rect, qreal dpr)
{
    if (rect.isEmpty() || KWindowSystem::isPlatformWayland()) {
        return rect;
    }

    // Make the position scaled like Wayland.
    return {rect.topLeft() / dpr, rect.size()};
}

QRectF Geometry::logicalScreensRect()
{
    QRectF rect;
    const auto &screens = qGuiApp->screens();
    for (int i = 0; i < screens.size(); ++i) {
        rect |= Geometry::mapFromPlatformRect(screens[i]->geometry(), qGuiApp->devicePixelRatio());
    }
    return rect;
}

qreal Geometry::mapToPlatformValue(qreal value, qreal dpr)
{
    if (KWindowSystem::isPlatformX11()) {
        return value * dpr;
    }
    return value;
}

QPointF Geometry::mapToPlatformPoint(const QPointF &point, qreal dpr)
{
    if (KWindowSystem::isPlatformX11()) {
        return point * dpr;
    }
    return point;
}

QRectF Geometry::mapToPlatformRect(const QRectF &rect, qreal dpr)
{
    if (rect.isEmpty() || KWindowSystem::isPlatformWayland()) {
        return rect;
    }
    return {rect.topLeft() * dpr, rect.size()};
}

QRectF Geometry::platformUnifiedRect()
{
    QRectF rect;
    const auto &screens = qGuiApp->screens();
    for (int i = 0; i < screens.size(); ++i) {
        rect |= screens[i]->geometry();
    }
    return rect;
}

QSize Geometry::rawSize(const QSizeF &size, qreal dpr)
{
    return (size * dpr).toSize();
}

QRectF Geometry::rectNormalized(const QRectF &rect)
{
    return rect.normalized();
}

QRectF Geometry::rectNormalized(qreal x, qreal y, qreal w, qreal h)
{
    return QRectF(x, y, w, h).normalized();
}

QRectF Geometry::rectAdjusted(const QRectF &rect, qreal xp1, qreal yp1, qreal xp2, qreal yp2)
{
    return rect.adjusted(xp1, yp1, xp2, yp2);
}

QRectF Geometry::rectAdjustedVisually(const QRectF &rect, qreal xp1, qreal yp1, qreal xp2, qreal yp2)
{
    if (rect.width() < 0) {
        std::swap(xp1, xp2);
    }
    if (rect.height() < 0) {
        std::swap(yp1, yp2);
    }
    return rect.adjusted(xp1, yp1, xp2, yp2);
}

QRectF Geometry::rectScaled(const QRectF &rect, qreal scale)
{
    if (scale == 1) {
        return rect;
    }
    return {rect.topLeft() * scale, rect.size() * scale};
}

QRectF Geometry::rectIntersected(const QRectF &rect1, const QRectF &rect2)
{
    return rect1.intersected(rect2);
}

QRectF Geometry::rectBounded(const QRectF &rect, const QRectF &boundsRect,
                             Qt::Orientations orientations)
{
    if (rect == boundsRect) {
        return rect;
    }
    auto newRect = rect;
    const auto &nBoundsRect = boundsRect.normalized(); // normalize to make math easier
    if (orientations & Qt::Horizontal) {
        if (rect.width() >= 0) {
            newRect.moveLeft(std::clamp(rect.x(), nBoundsRect.x(), nBoundsRect.right() - rect.width()));
        } else {
            newRect.moveRight(std::clamp(rect.right(), nBoundsRect.x(), nBoundsRect.right() - std::abs(rect.width())));
        }
    }
    if (orientations & Qt::Vertical) {
        if (rect.height() >= 0) {
            newRect.moveTop(std::clamp(rect.y(), nBoundsRect.y(), nBoundsRect.bottom() - std::abs(rect.height())));
        } else {
            newRect.moveBottom(std::clamp(rect.bottom(), nBoundsRect.y(), nBoundsRect.bottom() - std::abs(rect.height())));
        }
    }
    return newRect;
}

QRectF Geometry::rectBounded(qreal x, qreal y, qreal width, qreal height, const QRectF &boundsRect,
                             Qt::Orientations orientations)
{
    return rectBounded({x, y, width, height}, boundsRect, orientations);
}

void translateXFromIntersection(QRectF &rect, QPointF &point, qreal side, auto setter)
{
    if (point.x() < rect.center().x()) {
        rect.*setter(side + point.x());
    } else {
        rect.*setter(side + point.x());
    }
}

void translateYFromIntersection(QRectF &rect, QPointF &point)
{
    if (point.y() < rect.center().y()) {
        rect.moveLeft(rect.left() - point.y());
    } else {
        rect.moveRight(rect.right() - point.y());
    }
}

QRectF Geometry::rectBounded(const QRect &rect, const QPainterPath &boundsPath)
{
    if (boundsPath.contains(rect) && boundsPath.intersects(rect)) {
        return rect;
    }
    auto newRect = rect;
    QPainterPath::Element last = boundsPath.elementAt(0);
    for (int i = 1; i < boundsPath.elementCount(); ++i) {
        auto e = boundsPath.elementAt(i);
        // straight line closed shapes only
        QLineF line{last, e};
        QLineF left{newRect.topLeft(), newRect.bottomLeft()};
        QPointF leftIntersection;
        QLineF top{newRect.topLeft(), newRect.topRight()};
        QPointF topIntersection;
        QLineF right{newRect.topRight(), newRect.bottomRight()};
        QPointF rightIntersection;
        QLineF bottom{newRect.bottomLeft(), newRect.bottomLeft()};
        QPointF bottomIntersection;
        if (line.intersects(left, &leftIntersection)) {
            if (newRect.left() < leftIntersection.x()) {
                newRect.moveLeft(leftIntersection.x());
            } else {
                newRect.moveLeft(leftIntersection.x());
            }
        }
        if (line.intersects(top, &topIntersection)) {
            if (newRect.top() < topIntersection.y()) {
                newRect.moveTop(topIntersection.y());
            } else {
                newRect.moveTop(topIntersection.y());
            }
        }
        if (line.intersects(right, &rightIntersection)) {
            if (newRect.right() < leftIntersection.x()) {
                newRect.moveRight(leftIntersection.x());
            } else {
                newRect.moveRight(leftIntersection.x());
            }
        }
        if (line.intersects(bottom, &bottomIntersection)) {
            if (newRect.bottom() < topIntersection.y()) {
                newRect.moveBottom(topIntersection.y());
            } else {
                newRect.moveBottom(topIntersection.y());
            }
        }
        last = e;
    }
    return newRect;
}

QRectF Geometry::rectClipped(const QRectF &rect, const QRectF &clipRect,
                             Qt::Orientations orientations)
{
    if (rect == clipRect) {
        return rect;
    }
    auto newRect = rect;
    const auto &nClipRect = clipRect.normalized(); // normalize to make math easier
    if (orientations & Qt::Horizontal) {
        if (rect.width() >= 0) {
            newRect.setLeft(std::max(rect.x(), nClipRect.x()));
            newRect.setRight(std::min(rect.right(), nClipRect.right()));
        } else {
            newRect.setLeft(std::min(rect.x(), nClipRect.right()));
            newRect.setRight(std::max(rect.right(), nClipRect.x()));
        }
    }
    if (orientations & Qt::Vertical) {
        if (rect.height() >= 0) {
            newRect.setTop(std::max(rect.y(), nClipRect.y()));
            newRect.setBottom(std::min(rect.bottom(), nClipRect.bottom()));
        } else {
            newRect.setTop(std::min(rect.y(), nClipRect.bottom()));
            newRect.setBottom(std::max(rect.bottom(), nClipRect.y()));
        }
    }
    return newRect;
}

bool Geometry::rectContains(const QRectF &rect, qreal v, Qt::Orientations orientations)
{
    Q_ASSERT(orientations != Qt::Orientations{});
    bool contains = true;
    if (orientations.testFlag(Qt::Horizontal)) {
        contains &= std::min(rect.left(), rect.right()) <= v && v <= std::max(rect.left(), rect.right());
    }
    if (contains && orientations.testFlag(Qt::Vertical)) {
        contains &= std::min(rect.top(), rect.bottom()) <= v && v <= std::max(rect.top(), rect.bottom());
    }
    return contains;
}

bool Geometry::rectContains(const QRectF &rect, qreal x, qreal y)
{
    return rect.contains(x, y);
}

bool Geometry::rectContains(const QRectF &rect, const QPointF &point)
{
    return rect.contains(point);
}

bool Geometry::rectContains(const QRectF &rect1, const QRectF& rect2)
{
    return rect1.contains(rect2);
}

bool Geometry::rectContains(const QRectF &rect, qreal x, qreal y, qreal w, qreal h)
{
    return rect.contains({x, y, w, h});
}

bool Geometry::ellipseContains(qreal ellipseX, qreal ellipseY,
                               qreal ellipseWidth, qreal ellipseHeight,
                               qreal x, qreal y)
{
    if (isEmpty(ellipseWidth, ellipseHeight)) {
        return false;
    }
    auto xRadius = ellipseWidth / 2;
    auto yRadius = ellipseHeight / 2;
    auto centerX = ellipseX + xRadius;
    auto centerY = ellipseY + yRadius;
    // is inside or on edge
    return std::pow(x - centerX, 2) / std::pow(xRadius, 2)
         + std::pow(y - centerY, 2) / std::pow(yRadius, 2) <= 1;
}

bool Geometry::ellipseContains(const QRectF &rect, qreal x, qreal y)
{
    return ellipseContains(rect.x(), rect.y(), rect.width(), rect.height(), x, y);
}

bool Geometry::ellipseContains(const QRectF &rect, const QPointF &point)
{
    return ellipseContains(rect, point.x(), point.y());
}

bool Geometry::rectIntersects(const QRectF &rect1, const QRectF& rect2)
{
    return rect1.intersects(rect2);
}

bool Geometry::rectIntersects(const QRectF &rect, qreal x, qreal y, qreal w, qreal h)
{
    return rect.intersects({x, y, w, h});
}

bool Geometry::isEmpty(qreal w, qreal h)
{
    return w <= 0 || h <= 0;
}

bool Geometry::isEmpty(const QSizeF &size)
{
    return size.isEmpty();
}

bool Geometry::isEmpty(const QRectF &rect)
{
    return rect.isEmpty();
}

bool Geometry::isNull(qreal w, qreal h)
{
    return qIsNull(w) && qIsNull(h);
}

bool Geometry::isNull(const QSizeF &size)
{
    return size.isNull();
}

bool Geometry::isNull(const QRectF &rect)
{
    return rect.isNull();
}

#include <moc_Geometry.cpp>
