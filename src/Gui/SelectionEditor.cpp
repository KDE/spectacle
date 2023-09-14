/*
 *  SPDX-FileCopyrightText: 2018 Ambareesh "Amby" Balaji <ambareeshbalaji@gmail.com>
 *  SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SelectionEditor.h"

#include "Selection.h"
#include "Geometry.h"
#include "settings.h"
#include "spectacle_gui_debug.h"

#include <KLocalizedString>
#include <KWindowSystem>

#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPixmapCache>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QtMath>
#include <qnamespace.h>

using G = Geometry;

class SelectionEditorSingleton
{
public:
    SelectionEditor self;
};

Q_GLOBAL_STATIC(SelectionEditorSingleton, privateSelectionEditorSelf)

static constexpr qreal s_handleRadiusMouse = 9;
static constexpr qreal s_handleRadiusTouch = 12;
static constexpr qreal s_minSpacingBetweenHandles = 20;
static constexpr qreal s_magnifierLargeStep = 15;

// Map the global position of the scene to a logical global position (if necessary),
// then translate the scene position of the event to a logical global position using
// the logical global position of the scene. If you try to get the global event position
// and then convert it to a logical global position, you will get the wrong mouse position
// since the event position in the scene is already scaled by the scene's device pixel ratio.
QPointF mapSceneToLogicalGlobalPoint(const QPointF &point, QQuickItem *item)
{
    auto window = item ? item->window() : nullptr;
    Q_ASSERT(window != nullptr);
    return point + G::mapFromPlatformPoint(window->position(), window->devicePixelRatio());
}

// SelectionEditorPrivate =====================

using MouseLocation = SelectionEditor::MouseLocation;

class SelectionEditorPrivate
{
public:
    SelectionEditorPrivate(SelectionEditor *q);

    SelectionEditor *const q;

    void updateHandlePositions();

    qreal dprRound(qreal value) const;

    void handleArrowKey(QKeyEvent *event);

    void setMouseCursor(QQuickItem *item, const QPointF &pos);
    MouseLocation mouseLocation(const QPointF &pos) const;

    const std::unique_ptr<Selection> selection;

    QPointF startPos;
    QPointF initialTopLeft;
    MouseLocation dragLocation = MouseLocation::None;
    qreal devicePixelRatio = 1;
    qreal devicePixel = 1;
    QPointF mousePos;
    bool magnifierAllowed = false;
    bool toggleMagnifier = false;
    bool disableArrowKeys = false;
    QRectF screensRect;
    // Midpoints of handles
    QVector<QPointF> handlePositions = QVector<QPointF>{8};
    QRectF handlesRect;
    // Radius of handles is either handleRadiusMouse or handleRadiusTouch
    qreal handleRadius = s_handleRadiusMouse;
    qreal penWidth = 1;
    qreal penOffset = 0.5;
};

SelectionEditorPrivate::SelectionEditorPrivate(SelectionEditor *q)
    : q(q)
    , selection(new Selection(q))
{
}

void SelectionEditorPrivate::updateHandlePositions()
{
    // Rectangular region
    const qreal left = selection->left();
    const qreal centerX = selection->horizontalCenter();
    const qreal right = selection->right();
    const qreal top = selection->top();
    const qreal centerY = selection->verticalCenter();
    const qreal bottom = selection->bottom();

    // rectangle too small: make handles free-floating
    qreal offset = 0;
    // rectangle too close to screen edges: move handles on that edge inside the rectangle, so they're still visible
    qreal offsetTop = 0;
    qreal offsetRight = 0;
    qreal offsetBottom = 0;
    qreal offsetLeft = 0;

    const qreal minDragHandleSpace = 4 * handleRadius + 2 * s_minSpacingBetweenHandles;
    const qreal minEdgeLength = qMin(selection->width(), selection->height());
    if (minEdgeLength < minDragHandleSpace) {
        offset = (minDragHandleSpace - minEdgeLength) / 2.0;
    } else {
        const auto translatedScreensRect = screensRect.translated(-screensRect.topLeft());

        offsetTop = top - translatedScreensRect.top() - handleRadius;
        offsetTop = (offsetTop >= 0) ? 0 : offsetTop;

        offsetRight = translatedScreensRect.right() - right - handleRadius + penWidth;
        offsetRight = (offsetRight >= 0) ? 0 : offsetRight;

        offsetBottom = translatedScreensRect.bottom() - bottom - handleRadius + penWidth;
        offsetBottom = (offsetBottom >= 0) ? 0 : offsetBottom;

        offsetLeft = left - translatedScreensRect.left() - handleRadius;
        offsetLeft = (offsetLeft >= 0) ? 0 : offsetLeft;
    }

    // top-left handle
    handlePositions[0] = QPointF{left - offset - offsetLeft, top - offset - offsetTop};
    // top-right handle
    handlePositions[1] = QPointF{right + offset + offsetRight, top - offset - offsetTop};
    // bottom-right handle
    handlePositions[2] = QPointF{right + offset + offsetRight, bottom + offset + offsetBottom};
    // bottom-left
    handlePositions[3] = QPointF{left - offset - offsetLeft, bottom + offset + offsetBottom};
    // top-center handle
    handlePositions[4] = QPointF{centerX, top - offset - offsetTop};
    // right-center handle
    handlePositions[5] = QPointF{right + offset + offsetRight, centerY};
    // bottom-center handle
    handlePositions[6] = QPointF{centerX, bottom + offset + offsetBottom};
    // left-center handle
    handlePositions[7] = QPointF{left - offset - offsetLeft, centerY};

    QPointF radiusOffset = {handleRadius, handleRadius};
    QRectF newHandlesRect = {handlePositions[0] - radiusOffset, // top left
                             handlePositions[2] + radiusOffset}; // bottom right
    if (handlesRect == newHandlesRect) {
        return;
    }
    handlesRect = newHandlesRect;
    Q_EMIT q->handlesRectChanged();
}

qreal SelectionEditorPrivate::dprRound(qreal value) const
{
    return G::dprRound(value, devicePixelRatio);
}

void SelectionEditorPrivate::handleArrowKey(QKeyEvent *event)
{
    if (disableArrowKeys) {
        return;
    }

    const auto key = static_cast<Qt::Key>(event->key());
    const auto modifiers = event->modifiers();
    const bool modifySize = modifiers & Qt::AltModifier;
    const qreal step = modifiers & Qt::ShiftModifier ? devicePixel : dprRound(s_magnifierLargeStep);
    QRectF selectionRect = selection->rectF();

    if (key == Qt::Key_Left) {
        if (modifySize) {
            selectionRect = G::rectAdjustedVisually(selectionRect, 0, 0, -step, 0);
        } else {
            selectionRect.translate(-step, 0);
        }
    } else if (key == Qt::Key_Right) {
        if (modifySize) {
            selectionRect = G::rectAdjustedVisually(selectionRect, 0, 0, step, 0);
        } else {
            selectionRect.translate(step, 0);
        }
    } else if (key == Qt::Key_Up) {
        if (modifySize) {
            selectionRect = G::rectAdjustedVisually(selectionRect, 0, 0, 0, -step);
        } else {
            selectionRect.translate(0, -step);
        }
    } else if (key == Qt::Key_Down) {
        if (modifySize) {
            selectionRect = G::rectAdjustedVisually(selectionRect, 0, 0, 0, step);
        } else {
            selectionRect.translate(0, step);
        }
    }
    selection->setRect(modifySize ? selectionRect : G::rectBounded(selectionRect, screensRect));
}

// TODO: change cursor with pointerhandlers in qml?
void SelectionEditorPrivate::setMouseCursor(QQuickItem *item, const QPointF &pos)
{
    MouseLocation mouseState = mouseLocation(pos);
    if (mouseState == MouseLocation::Outside) {
        item->setCursor(Qt::CrossCursor);
    } else if (MouseLocation::TopLeftOrBottomRight & mouseState) {
        item->setCursor(Qt::SizeFDiagCursor);
    } else if (MouseLocation::TopRightOrBottomLeft & mouseState) {
        item->setCursor(Qt::SizeBDiagCursor);
    } else if (MouseLocation::TopOrBottom & mouseState) {
        item->setCursor(Qt::SizeVerCursor);
    } else if (MouseLocation::RightOrLeft & mouseState) {
        item->setCursor(Qt::SizeHorCursor);
    } else {
        item->setCursor(Qt::OpenHandCursor);
    }
}

SelectionEditor::MouseLocation SelectionEditorPrivate::mouseLocation(const QPointF &pos) const
{
    QRectF handleRect(-handleRadius, -handleRadius, handleRadius * 2, handleRadius * 2);
    if (G::ellipseContains(handleRect.translated(handlePositions[0]), pos)) {
        return MouseLocation::TopLeft;
    }
    if (G::ellipseContains(handleRect.translated(handlePositions[1]), pos)) {
        return MouseLocation::TopRight;
    }
    if (G::ellipseContains(handleRect.translated(handlePositions[2]), pos)) {
        return MouseLocation::BottomRight;
    }
    if (G::ellipseContains(handleRect.translated(handlePositions[3]), pos)) {
        return MouseLocation::BottomLeft;
    }
    if (G::ellipseContains(handleRect.translated(handlePositions[4]), pos)) {
        return MouseLocation::Top;
    }
    if (G::ellipseContains(handleRect.translated(handlePositions[5]), pos)) {
        return MouseLocation::Right;
    }
    if (G::ellipseContains(handleRect.translated(handlePositions[6]), pos)) {
        return MouseLocation::Bottom;
    }
    if (G::ellipseContains(handleRect.translated(handlePositions[7]), pos)) {
        return MouseLocation::Left;
    }

    const auto rect = selection->normalized();
    // Rectangle can be resized when border is dragged, if it's big enough
    if (rect.width() >= 100 && rect.height() >= 100) {
        if (rect.adjusted(0, -handleRadius, 0, -rect.height() + handleRadius).contains(pos)) {
            return MouseLocation::Top;
        }
        if (rect.adjusted(0, rect.height() - handleRadius, 0, handleRadius).contains(pos)) {
            return MouseLocation::Bottom;
        }
        if (rect.adjusted(-handleRadius, 0, -rect.width() + handleRadius, 0).contains(pos)) {
            return MouseLocation::Left;
        }
        if (rect.adjusted(rect.width() - handleRadius, 0, handleRadius, 0).contains(pos)) {
            return MouseLocation::Right;
        }
    }
    if (rect.contains(pos)) {
        return MouseLocation::Inside;
    }
    return MouseLocation::Outside;
}

// SelectionEditor =================================

SelectionEditor::SelectionEditor(QObject *parent)
    : QObject(parent)
    , d(new SelectionEditorPrivate(this))
{
    setObjectName(QStringLiteral("selectionEditor"));

    connect(d->selection.get(), &Selection::rectChanged, this, [this](){
        d->updateHandlePositions();
    });
}

SelectionEditor *SelectionEditor::instance()
{
    return &privateSelectionEditorSelf()->self;
}

Selection *SelectionEditor::selection() const
{
    return d->selection.get();
}

qreal SelectionEditor::devicePixelRatio() const
{
    return d->devicePixelRatio;
}

void SelectionEditor::setDevicePixelRatio(qreal dpr)
{
    if (d->devicePixelRatio == dpr) {
        return;
    }
    d->devicePixelRatio = dpr;
    d->devicePixel = 1 / dpr;
    Q_EMIT devicePixelRatioChanged();
}

QRectF SelectionEditor::screensRect() const
{
    return d->screensRect;
}

void SelectionEditor::setScreensRect(const QRectF &rect)
{
    if (d->screensRect == rect) {
        return;
    }
    d->screensRect = rect;
    Q_EMIT screensRectChanged();
}

qreal SelectionEditor::screensWidth() const
{
    return d->screensRect.width();
}

qreal SelectionEditor::screensHeight() const
{
    return d->screensRect.height();
}

MouseLocation SelectionEditor::dragLocation() const
{
    return d->dragLocation;
}

QRectF SelectionEditor::handlesRect() const
{
    return d->handlesRect;
}

bool SelectionEditor::magnifierAllowed() const
{
    return d->magnifierAllowed;
}

QPointF SelectionEditor::mousePosition() const
{
    return d->mousePos;
}

bool SelectionEditor::acceptSelection(ExportManager::Actions actions)
{
    if (d->screensRect.isEmpty()) {
        return false;
    }

    auto selectionRect = d->selection->normalized();
    if (Settings::rememberLastRectangularRegion() == Settings::Always) {
        Settings::setCropRegion(selectionRect.toAlignedRect());
    }

    if (selectionRect.isEmpty()) {
        selectionRect = d->screensRect;
    }

    Q_EMIT accepted(selectionRect, actions);
    return true;
}

bool SelectionEditor::eventFilter(QObject *watched, QEvent *event)
{
    auto *item = qobject_cast<QQuickItem *>(watched);

    if (!item) {
        return false;
    }

    switch (event->type()) {
    case QEvent::KeyPress:
        keyPressEvent(item, static_cast<QKeyEvent *>(event));
        break;
    case QEvent::KeyRelease:
        keyReleaseEvent(item, static_cast<QKeyEvent *>(event));
        break;
    case QEvent::HoverMove:
        hoverMoveEvent(item, static_cast<QHoverEvent *>(event));
        break;
    case QEvent::MouseButtonPress:
        mousePressEvent(item, static_cast<QMouseEvent *>(event));
        break;
    case QEvent::MouseMove:
        mouseMoveEvent(item, static_cast<QMouseEvent *>(event));
        break;
    case QEvent::MouseButtonRelease:
        mouseReleaseEvent(item, static_cast<QMouseEvent *>(event));
        break;
    case QEvent::MouseButtonDblClick:
        mouseDoubleClickEvent(item, static_cast<QMouseEvent *>(event));
        break;
    default:
        break;
    }
    return false;
}

void SelectionEditor::keyPressEvent(QQuickItem *item, QKeyEvent *event)
{
    Q_UNUSED(item);

    const auto modifiers = event->modifiers();
    const bool shiftPressed = modifiers & Qt::ShiftModifier;
    if (shiftPressed) {
        d->toggleMagnifier = true;
    }
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        acceptSelection();
        event->accept();
        break;
    case Qt::Key_Up:
    case Qt::Key_Right:
    case Qt::Key_Down:
    case Qt::Key_Left:
        d->handleArrowKey(event);
        event->accept();
        break;
    default:
        break;
    }
}

void SelectionEditor::keyReleaseEvent(QQuickItem *item, QKeyEvent *event)
{
    Q_UNUSED(item);

    if (d->toggleMagnifier && !(event->modifiers() & Qt::ShiftModifier)) {
        d->toggleMagnifier = false;
    }
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        event->accept();
        break;
    case Qt::Key_Up:
    case Qt::Key_Right:
    case Qt::Key_Down:
    case Qt::Key_Left:
        event->accept();
        break;
    default:
        break;
    }
}

void SelectionEditor::hoverMoveEvent(QQuickItem *item, QHoverEvent *event)
{
    if (!item->window() || !item->window()->screen()) {
        return;
    }
    d->mousePos = mapSceneToLogicalGlobalPoint(item->mapToScene(event->posF()), item);
    Q_EMIT mousePositionChanged();
    d->setMouseCursor(item, d->mousePos);
}

void SelectionEditor::mousePressEvent(QQuickItem *item, QMouseEvent *event)
{
    if (!item->window() || !item->window()->screen()) {
        return;
    }

    if (event->source() == Qt::MouseEventNotSynthesized) {
        d->handleRadius = s_handleRadiusMouse;
    } else {
        d->handleRadius = s_handleRadiusTouch;
    }

    if (event->button() & (Qt::LeftButton | Qt::RightButton)) {
        if (event->button() & Qt::RightButton) {
            d->selection->setRect({});
        }
        item->setFocus(true);
        const bool wasMagnifierAllowed = d->magnifierAllowed;
        d->mousePos = mapSceneToLogicalGlobalPoint(event->windowPos(), item);
        Q_EMIT mousePositionChanged();
        auto newDragLocation = d->mouseLocation(d->mousePos);
        if (d->dragLocation != newDragLocation) {
            d->dragLocation = newDragLocation;
            Q_EMIT dragLocationChanged();
        }
        d->magnifierAllowed = true;
        d->disableArrowKeys = true;

        switch (d->dragLocation) {
        case MouseLocation::Outside:
            d->startPos = d->mousePos;
            break;
        case MouseLocation::Inside:
            d->startPos = d->mousePos;
            d->magnifierAllowed = false;
            d->initialTopLeft = d->selection->rectF().topLeft();
            item->setCursor(Qt::ClosedHandCursor);
            break;
        case MouseLocation::Top:
        case MouseLocation::Left:
        case MouseLocation::TopLeft:
            d->startPos = d->selection->rectF().bottomRight();
            break;
        case MouseLocation::Bottom:
        case MouseLocation::Right:
        case MouseLocation::BottomRight:
            d->startPos = d->selection->rectF().topLeft();
            break;
        case MouseLocation::TopRight:
            d->startPos = d->selection->rectF().bottomLeft();
            break;
        case MouseLocation::BottomLeft:
            d->startPos = d->selection->rectF().topRight();
            break;
        default:
            break;
        }

        if (d->magnifierAllowed != wasMagnifierAllowed) {
            Q_EMIT magnifierAllowedChanged();
        }
    }
    event->accept();
}

void SelectionEditor::mouseMoveEvent(QQuickItem *item, QMouseEvent *event)
{
    if (!item->window() || !item->window()->screen()) {
        return;
    }

    d->mousePos = mapSceneToLogicalGlobalPoint(event->windowPos(), item);
    Q_EMIT mousePositionChanged();
    const bool wasMagnifierAllowed = d->magnifierAllowed;
    d->magnifierAllowed = true;
    switch (d->dragLocation) {
    case MouseLocation::None: {
        d->setMouseCursor(item, d->mousePos);
        d->magnifierAllowed = false;
        break;
    }
    case MouseLocation::TopLeft:
    case MouseLocation::TopRight:
    case MouseLocation::BottomRight:
    case MouseLocation::BottomLeft: {
        const bool afterX = d->mousePos.x() >= d->startPos.x();
        const bool afterY = d->mousePos.y() >= d->startPos.y();
        d->selection->setRect(afterX ? d->startPos.x() : d->mousePos.x(),
                              afterY ? d->startPos.y() : d->mousePos.y(),
                              qAbs(d->mousePos.x() - d->startPos.x()) + (afterX ? d->devicePixel : 0),
                              qAbs(d->mousePos.y() - d->startPos.y()) + (afterY ? d->devicePixel : 0));
        break;
    }
    case MouseLocation::Outside: {
        d->selection->setRect(qMin(d->mousePos.x(), d->startPos.x()),
                              qMin(d->mousePos.y(), d->startPos.y()),
                              qAbs(d->mousePos.x() - d->startPos.x()) + d->devicePixel,
                              qAbs(d->mousePos.y() - d->startPos.y()) + d->devicePixel);
        break;
    }
    case MouseLocation::Top:
    case MouseLocation::Bottom: {
        const bool afterY = d->mousePos.y() >= d->startPos.y();
        d->selection->setRect(d->selection->x(),
                              afterY ? d->startPos.y() : d->mousePos.y(),
                              d->selection->width(),
                              qAbs(d->mousePos.y() - d->startPos.y()) + (afterY ? d->devicePixel : 0));
        break;
    }
    case MouseLocation::Right:
    case MouseLocation::Left: {
        const bool afterX = d->mousePos.x() >= d->startPos.x();
        d->selection->setRect(afterX ? d->startPos.x() : d->mousePos.x(),
                              d->selection->y(),
                              qAbs(d->mousePos.x() - d->startPos.x()) + (afterX ? d->devicePixel : 0),
                              d->selection->height());
        break;
    }
    case MouseLocation::Inside: {
        d->magnifierAllowed = false;
        // We use some math here to figure out if the diff with which we move the rectangle
        QRectF newRect(d->mousePos - d->startPos + d->initialTopLeft, d->selection->sizeF());
        d->selection->setRect(G::rectBounded(newRect, d->screensRect));
        break;
    }
    default:
        break;
    }
    if (d->magnifierAllowed != wasMagnifierAllowed) {
        Q_EMIT magnifierAllowedChanged();
    }

    event->accept();
}

void SelectionEditor::mouseReleaseEvent(QQuickItem *item, QMouseEvent *event)
{
    switch (event->button()) {
    case Qt::LeftButton:
    case Qt::RightButton:
        if (d->dragLocation == MouseLocation::Outside && Settings::useReleaseToCapture()) {
            acceptSelection();
            return;
        }
        d->disableArrowKeys = false;
        if (d->dragLocation == MouseLocation::Inside) {
            item->setCursor(Qt::OpenHandCursor);
        }
        break;
    default:
        break;
    }
    event->accept();
    if (d->dragLocation != MouseLocation::None) {
        d->dragLocation = MouseLocation::None;
        Q_EMIT dragLocationChanged();
    }
    if (d->magnifierAllowed) {
        d->magnifierAllowed = false;
        Q_EMIT magnifierAllowedChanged();
    }
}

void SelectionEditor::mouseDoubleClickEvent(QQuickItem *item, QMouseEvent *event)
{
    Q_UNUSED(item)
    if (event->button() == Qt::LeftButton && d->selection->contains(d->mousePos)) {
        acceptSelection();
    }
    event->accept();
}

#include <moc_SelectionEditor.cpp>
