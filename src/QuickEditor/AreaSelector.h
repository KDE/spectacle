/*
 *  SPDX-FileCopyrightText: 2018 Ambareesh "Amby" Balaji <ambareeshbalaji@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QGraphicsItem>
#include <QObject>

class AreaSelectorItem : public QObject, public QAbstractGraphicsShapeItem
{
    Q_OBJECT

public:
    explicit AreaSelectorItem(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    QRectF selection() const;
    void setSelection(const QRectF &selection);

    QColor selectionColor() const;
    void setSelectionColor(const QColor &color);

    QRectF rect() const;
    void setRect(const QRectF &rect);

Q_SIGNALS:
    void selectionChanged();
    void selectionCanceled();
    void selectionAccepted();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    enum class MouseState {
        None,
        Inside,
        Outside,
        TopLeft,
        Top,
        TopRight,
        Right,
        BottomRight,
        Bottom,
        BottomLeft,
        Left,
    };

    MouseState mouseLocation(const QPointF &pos);
    void updateCursor(const QPointF &pos);
    void updateHandles();
    QRectF ensureInsideScene(const QRectF &rect) const;

    QRectF mSelection;
    QRectF mRect;

    QPointF mMousePos;
    QPointF mStartPos;
    QPointF mInitialTopLeft;
    MouseState mMouseDragState = MouseState::None;

    QVector<QGraphicsEllipseItem *> mHandles;
    int mHandleRadius = 0;
    QColor mSelectionColor;
};
