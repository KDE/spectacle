/* SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "QmlPainterPath.h"
#include <QMatrix4x4>

QString QmlPainterPath::toString() const
{
    return QStringLiteral("QPainterPath(%1)").arg(svgPath());
}

bool QmlPainterPath::contains(const QPointF &point) const
{
    return m_path.contains(point);
}

bool QmlPainterPath::contains(qreal x, qreal y) const
{
    return m_path.contains({x, y});
}

bool QmlPainterPath::contains(const QRectF &rect) const
{
    return m_path.contains(rect);
}

bool QmlPainterPath::intersects(const QRectF &rect) const
{
    return m_path.intersects(rect);
}

QPainterPath QmlPainterPath::map(const QMatrix4x4 &transform) const
{
    return transform.toTransform().map(m_path);
}

QRectF QmlPainterPath::mapBoundingRect(const QMatrix4x4 &transform) const
{
    return transform.toTransform().mapRect(m_path.boundingRect());
}

QString QmlPainterPath::toSvgPathElement(const QPainterPath::Element &element)
{
    switch (element.type) {
    case QPainterPath::MoveToElement:
        return QStringLiteral("M %1,%2").arg(element.x, 0, 'f').arg(element.y, 0, 'f');
    case QPainterPath::LineToElement:
        return QStringLiteral("L %1,%2").arg(element.x, 0, 'f').arg(element.y, 0, 'f');
    case QPainterPath::CurveToElement:
        return QStringLiteral("C %1,%2").arg(element.x, 0, 'f').arg(element.y, 0, 'f');
    case QPainterPath::CurveToDataElement:
        return QStringLiteral("%1,%2").arg(element.x, 0, 'f').arg(element.y, 0, 'f');
    }
    return {};
}

QString QmlPainterPath::toSvgPath(const QPainterPath &path)
{
    QString svgPath;
    for (int i = 0; i < path.elementCount(); ++i) {
        svgPath.append(toSvgPathElement(path.elementAt(i)) % u' ');
    }
    return svgPath;
}

QString QmlPainterPath::svgPath() const
{
    return toSvgPath(m_path);
}

bool QmlPainterPath::empty() const
{
    return m_path.isEmpty();
}

int QmlPainterPath::elementCount() const
{
    return m_path.elementCount();
}

QPointF QmlPainterPath::start() const
{
    return m_path.elementCount() > 0 ? m_path.elementAt(0) : QPointF{};
}

QPointF QmlPainterPath::end() const
{
    return m_path.currentPosition();
}

QRectF QmlPainterPath::boundingRect() const
{
    return m_path.boundingRect();
}

QmlPainterPath::operator QPainterPath() const
{
    return m_path;
}

#include "moc_QmlPainterPath.cpp"
