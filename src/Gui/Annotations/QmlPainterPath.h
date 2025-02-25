/* SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QPainterPath>
#include <QQmlEngine>

/**
 * A class for making QPainterPaths available in QML.
 */
class QmlPainterPath
{
    QPainterPath m_path;
    Q_GADGET

    /**
     * The path in the form of an SVG path string for use with a Qt Quick SvgPath.
     */
    Q_PROPERTY(QString svgPath READ svgPath FINAL)

    Q_PROPERTY(bool empty READ empty FINAL)
    Q_PROPERTY(int elementCount READ elementCount FINAL)
    Q_PROPERTY(QPointF start READ start FINAL)
    Q_PROPERTY(QPointF end READ end FINAL)
    Q_PROPERTY(QRectF boundingRect READ boundingRect FINAL)

    QML_ELEMENT
    QML_FOREIGN(QPainterPath)
    QML_EXTENDED(QmlPainterPath)

public:
    Q_INVOKABLE QString toString() const;

    Q_INVOKABLE bool contains(const QPointF &point) const;

    Q_INVOKABLE bool contains(qreal x, qreal y) const;

    Q_INVOKABLE bool contains(const QRectF &rect) const;

    Q_INVOKABLE bool intersects(const QRectF &rect) const;

    Q_INVOKABLE QPainterPath map(const QMatrix4x4 &transform) const;

    Q_INVOKABLE QRectF mapBoundingRect(const QMatrix4x4 &transform) const;

    static QString toSvgPathElement(const QPainterPath::Element &element);

    static QString toSvgPath(const QPainterPath &path);

    QString svgPath() const;

    bool empty() const;

    int elementCount() const;

    QPointF start() const;

    QPointF end() const;

    QRectF boundingRect() const;

    operator QPainterPath() const;
};
