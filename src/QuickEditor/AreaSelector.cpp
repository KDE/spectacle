/*
 *  SPDX-FileCopyrightText: 2018 Ambareesh "Amby" Balaji <ambareeshbalaji@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "AreaSelector.h"
#include "settings.h"

#include <QCursor>
#include <QGraphicsSceneHoverEvent>
#include <QKeyEvent>
#include <QPainter>

#include <cmath>

static const int gsHandleRadiusMouse = 9;
static const int gsHandleRadiusTouch = 12;
static const qreal gsIncreaseDragAreaFactor = 2.0;
static const int gsBorderDragAreaSize = 10;
static const int gsMagnifierLargeStep = 15;

AreaSelectorItem::AreaSelectorItem(QGraphicsItem *parent)
    : QAbstractGraphicsShapeItem(parent)
    , mHandleRadius(gsHandleRadiusMouse)
{
    for (int i = 0; i < 8; ++i) {
        QGraphicsEllipseItem *handleItem = new QGraphicsEllipseItem();
        mHandles.append(handleItem);
        handleItem->setPen(Qt::NoPen);
        handleItem->setVisible(true);
        handleItem->setParentItem(this);
    }

    setCursor(Qt::CrossCursor);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setAcceptHoverEvents(true);
    setFlag(ItemIsFocusable);
}

QRectF AreaSelectorItem::selection() const
{
    return mSelection;
}

void AreaSelectorItem::setSelection(const QRectF &selection)
{
    if (mSelection != selection) {
        const QRectF oldSelection = mSelection;
        mSelection = selection;
        update(oldSelection.united(selection));
        updateHandles();
        Q_EMIT selectionChanged();
    }
}

QColor AreaSelectorItem::selectionColor() const
{
    return mSelectionColor;
}

void AreaSelectorItem::setSelectionColor(const QColor &color)
{
    mSelectionColor = color;
    for (QGraphicsEllipseItem *handleItem : std::as_const(mHandles)) {
        handleItem->setBrush(color);
    }
}

QRectF AreaSelectorItem::boundingRect() const
{
    return mRect;
}

QRectF AreaSelectorItem::rect() const
{
    return mRect;
}

void AreaSelectorItem::setRect(const QRectF &rect)
{
    if (mRect != rect) {
        prepareGeometryChange();
        mRect = rect;
        update();
        updateHandles();
    }
}

QPainterPath AreaSelectorItem::shape() const
{
    QPainterPath path;
    path.addRect(mRect);
    return path;
}

void AreaSelectorItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    const QRectF selection = mSelection.normalized();

    QPainterPath path;
    path.addRect(mRect);
    if (!selection.isEmpty()) {
        QPainterPath cutout;
        cutout.addRect(selection);
        path -= cutout;
    }
    painter->fillPath(path, brush());

    if (!selection.isEmpty()) {
        painter->setPen(QColor::fromRgbF(mSelectionColor.redF(), mSelectionColor.greenF(), mSelectionColor.blueF(), 0.7));
        painter->drawRect(selection);
    }
}

void AreaSelectorItem::updateHandles()
{
    const qreal left = mSelection.x();
    const qreal centerX = left + mSelection.width() / 2.0;
    const qreal right = left + mSelection.width();
    const qreal top = mSelection.y();
    const qreal centerY = top + mSelection.height() / 2.0;
    const qreal bottom = top + mSelection.height();

    // top-left handle
    mHandles[0]->setRect(left - mHandleRadius, top - mHandleRadius, 2 * mHandleRadius, 2 * mHandleRadius);
    // top-right handle
    mHandles[1]->setRect(right - mHandleRadius, top - mHandleRadius, 2 * mHandleRadius, 2 * mHandleRadius);
    // bottom-right handle
    mHandles[2]->setRect(right - mHandleRadius, bottom - mHandleRadius, 2 * mHandleRadius, 2 * mHandleRadius);
    // bottom-left
    mHandles[3]->setRect(left - mHandleRadius, bottom - mHandleRadius, 2 * mHandleRadius, 2 * mHandleRadius);
    // top-center handle
    mHandles[4]->setRect(centerX - mHandleRadius, top - mHandleRadius, 2 * mHandleRadius, 2 * mHandleRadius);
    // right-center handle
    mHandles[5]->setRect(right - mHandleRadius, centerY - mHandleRadius, 2 * mHandleRadius, 2 * mHandleRadius);
    // bottom-center handle
    mHandles[6]->setRect(centerX - mHandleRadius, bottom - mHandleRadius, 2 * mHandleRadius, 2 * mHandleRadius);
    // left-center handle
    mHandles[7]->setRect(left - mHandleRadius, centerY - mHandleRadius, 2 * mHandleRadius, 2 * mHandleRadius);
}

void AreaSelectorItem::updateCursor(const QPointF &pos)
{
    switch (mouseLocation(pos)) {
    case MouseState::None:
        break;
    case MouseState::Outside:
        setCursor(Qt::CrossCursor);
        break;
    case MouseState::Inside:
        setCursor(Qt::OpenHandCursor);
        break;
    case MouseState::TopLeft:
    case MouseState::BottomRight:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case MouseState::TopRight:
    case MouseState::BottomLeft:
        setCursor(Qt::SizeBDiagCursor);
        break;
    case MouseState::Top:
    case MouseState::Bottom:
        setCursor(Qt::SizeVerCursor);
        break;
    case MouseState::Left:
    case MouseState::Right:
        setCursor(Qt::SizeHorCursor);
        break;
    }
}

AreaSelectorItem::MouseState AreaSelectorItem::mouseLocation(const QPointF &pos)
{
    auto isPointInsideCircle = [](const QGraphicsEllipseItem *item, qreal radius, const QPointF &point) {
        const QPointF circleCenter = item->rect().center();
        return (std::pow(point.x() - circleCenter.x(), 2) + std::pow(point.y() - circleCenter.y(), 2) <= std::pow(radius, 2)) ? true : false;
    };

    if (isPointInsideCircle(mHandles[0], mHandleRadius * gsIncreaseDragAreaFactor, pos)) {
        return MouseState::TopLeft;
    }
    if (isPointInsideCircle(mHandles[1], mHandleRadius * gsIncreaseDragAreaFactor, pos)) {
        return MouseState::TopRight;
    }
    if (isPointInsideCircle(mHandles[2], mHandleRadius * gsIncreaseDragAreaFactor, pos)) {
        return MouseState::BottomRight;
    }
    if (isPointInsideCircle(mHandles[3], mHandleRadius * gsIncreaseDragAreaFactor, pos)) {
        return MouseState::BottomLeft;
    }
    if (isPointInsideCircle(mHandles[4], mHandleRadius * gsIncreaseDragAreaFactor, pos)) {
        return MouseState::Top;
    }
    if (isPointInsideCircle(mHandles[5], mHandleRadius * gsIncreaseDragAreaFactor, pos)) {
        return MouseState::Right;
    }
    if (isPointInsideCircle(mHandles[6], mHandleRadius * gsIncreaseDragAreaFactor, pos)) {
        return MouseState::Bottom;
    }
    if (isPointInsideCircle(mHandles[7], mHandleRadius * gsIncreaseDragAreaFactor, pos)) {
        return MouseState::Left;
    }

    auto inRange = [](qreal low, qreal high, qreal value) {
        return value >= low && value <= high;
    };

    auto withinThreshold = [](qreal offset, qreal threshold) {
        return std::fabs(offset) <= threshold;
    };

    // Rectangle can be resized when border is dragged, if it's big enough
    if (mSelection.width() >= 100 && mSelection.height() >= 100) {
        if (inRange(mSelection.x(), mSelection.x() + mSelection.width(), pos.x())) {
            if (withinThreshold(pos.y() - mSelection.y(), gsBorderDragAreaSize)) {
                return MouseState::Top;
            }
            if (withinThreshold(pos.y() - mSelection.y() - mSelection.height(), gsBorderDragAreaSize)) {
                return MouseState::Bottom;
            }
        }
        if (inRange(mSelection.y(), mSelection.y() + mSelection.height(), pos.y())) {
            if (withinThreshold(pos.x() - mSelection.x(), gsBorderDragAreaSize)) {
                return MouseState::Left;
            }
            if (withinThreshold(pos.x() - mSelection.x() - mSelection.width(), gsBorderDragAreaSize)) {
                return MouseState::Right;
            }
        }
    }
    if (mSelection.contains(pos.toPoint())) {
        return MouseState::Inside;
    }
    return MouseState::Outside;
}

void AreaSelectorItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    updateCursor(event->pos());
}

QRectF AreaSelectorItem::ensureInsideScene(const QRectF &rect) const
{
    QRectF adjusted = rect;
    if (adjusted.left() < mRect.left()) {
        adjusted.moveLeft(mRect.left());
    } else if (adjusted.right() > mRect.right()) {
        adjusted.moveRight(mRect.right());
    }

    if (adjusted.top() < mRect.top()) {
        adjusted.moveTop(mRect.top());
    } else if (adjusted.bottom() > mRect.bottom()) {
        adjusted.moveBottom(mRect.bottom());
    }
    return adjusted;
}

void AreaSelectorItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    mMousePos = event->pos();
    switch (mMouseDragState) {
    case MouseState::None: {
        break;
    }
    case MouseState::TopLeft: {
        QRectF newSelection = selection();
        newSelection.setTopLeft(mMousePos);
        setSelection(newSelection);
        break;
    }
    case MouseState::TopRight: {
        QRectF newSelection = selection();
        newSelection.setTopRight(mMousePos);
        setSelection(newSelection);
        break;
    }
    case MouseState::BottomRight: {
        QRectF newSelection = selection();
        newSelection.setBottomRight(mMousePos);
        setSelection(newSelection);
        break;
    }
    case MouseState::BottomLeft: {
        QRectF newSelection = selection();
        newSelection.setBottomLeft(mMousePos);
        setSelection(newSelection);
        break;
    }
    case MouseState::Outside: {
        setSelection(QRectF(std::min(mMousePos.x(), mStartPos.x()),
                            std::min(mMousePos.y(), mStartPos.y()),
                            std::abs(mMousePos.x() - mStartPos.x()),
                            std::abs(mMousePos.y() - mStartPos.y())));
        break;
    }
    case MouseState::Top: {
        QRectF newSelection = selection();
        newSelection.setTop(mMousePos.y());
        setSelection(newSelection);
        break;
    }
    case MouseState::Bottom: {
        QRectF newSelection = selection();
        newSelection.setBottom(mMousePos.y());
        setSelection(newSelection);
        break;
    }
    case MouseState::Right: {
        QRectF newSelection = selection();
        newSelection.setRight(mMousePos.x());
        setSelection(newSelection);
        break;
    }
    case MouseState::Left: {
        QRectF newSelection = selection();
        newSelection.setLeft(mMousePos.x());
        setSelection(newSelection);
        break;
    }
    case MouseState::Inside: {
        QRectF newSelection = selection();
        newSelection.moveTopLeft(mMousePos - mStartPos + mInitialTopLeft);
        setSelection(ensureInsideScene(newSelection));
        break;
    }
    default:
        break;
    }

    event->accept();
}

void AreaSelectorItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->source() == Qt::MouseEventNotSynthesized) {
        mHandleRadius = gsHandleRadiusMouse;
    } else {
        mHandleRadius = gsHandleRadiusTouch;
    }

    if (event->button() & Qt::LeftButton) {
        mMousePos = event->pos();
        mMouseDragState = mouseLocation(mMousePos);
        switch (mMouseDragState) {
        case MouseState::Outside:
            mStartPos = mMousePos;
            break;
        case MouseState::Inside:
            mStartPos = mMousePos;
            mInitialTopLeft = mSelection.topLeft();
            setCursor(Qt::ClosedHandCursor);
            break;
        case MouseState::Top:
        case MouseState::Left:
        case MouseState::TopLeft:
            mStartPos = mSelection.bottomRight();
            break;
        case MouseState::Bottom:
        case MouseState::Right:
        case MouseState::BottomRight:
            mStartPos = mSelection.topLeft();
            break;
        case MouseState::TopRight:
            mStartPos = mSelection.bottomLeft();
            break;
        case MouseState::BottomLeft:
            mStartPos = mSelection.topRight();
            break;
        default:
            break;
        }
    }
    event->accept();
}

void AreaSelectorItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    switch (event->button()) {
    case Qt::LeftButton:
        if (mMouseDragState == MouseState::Inside) {
            setCursor(Qt::OpenHandCursor);
        }
        if (mMouseDragState == MouseState::Outside && Settings::useReleaseToCapture()) {
            Q_EMIT selectionAccepted();
        }
        break;
    case Qt::RightButton:
        setSelection(QRectF());
        break;
    default:
        break;
    }
    mMouseDragState = MouseState::None;
    event->accept();
}

void AreaSelectorItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    event->accept();
    if (event->button() == Qt::LeftButton && mSelection.contains(event->pos())) {
        Q_EMIT selectionAccepted();
    }
}

void AreaSelectorItem::keyPressEvent(QKeyEvent *event)
{
    const bool shiftPressed = event->modifiers() & Qt::ShiftModifier;

    switch (event->key()) {
    case Qt::Key_Escape:
        Q_EMIT selectionCanceled();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        Q_EMIT selectionAccepted();
        break;
    case Qt::Key_Up: {
        if (mMouseDragState != MouseState::None) {
            event->ignore();
            return;
        }
        const qreal step = (shiftPressed ? 1 : gsMagnifierLargeStep);
        const qreal newPos = selection().top() - step;
        QRectF newSelection = selection();
        if (event->modifiers() & Qt::AltModifier) {
            newSelection.setBottom(newPos + newSelection.height());
            newSelection = newSelection.normalized();
        } else {
            newSelection.moveTop(newPos);
        }
        setSelection(ensureInsideScene(newSelection));
        break;
    }
    case Qt::Key_Right: {
        if (mMouseDragState != MouseState::None) {
            event->ignore();
            return;
        }
        const qreal step = (shiftPressed ? 1 : gsMagnifierLargeStep);
        const int newPos = selection().left() + step;
        QRectF newSelection = selection();
        if (event->modifiers() & Qt::AltModifier) {
            newSelection.setRight(newPos + newSelection.width());
        } else {
            newSelection.moveLeft(newPos);
        }
        setSelection(ensureInsideScene(newSelection));
        break;
    }
    case Qt::Key_Down: {
        if (mMouseDragState != MouseState::None) {
            event->ignore();
            return;
        }
        const qreal step = (shiftPressed ? 1 : gsMagnifierLargeStep);
        const int newPos = selection().top() + step;
        QRectF newSelection = selection();
        if (event->modifiers() & Qt::AltModifier) {
            newSelection.setBottom(newPos + newSelection.height());
        } else {
            newSelection.moveTop(newPos);
        }
        setSelection(ensureInsideScene(newSelection));
        break;
    }
    case Qt::Key_Left: {
        if (mMouseDragState != MouseState::None) {
            event->ignore();
            return;
        }
        const qreal step = (shiftPressed ? 1 : gsMagnifierLargeStep);
        const int newPos = selection().left() - step;
        QRectF newSelection = selection();
        if (event->modifiers() & Qt::AltModifier) {
            newSelection.setRight(newPos + newSelection.width());
            newSelection = newSelection.normalized();
        } else {
            newSelection.moveLeft(newPos);
        }
        setSelection(ensureInsideScene(newSelection));
        break;
    }
    default:
        break;
    }
    event->accept();
}
