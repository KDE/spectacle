/*
 *  SPDX-FileCopyrightText: 2018 Ambareesh "Amby" Balaji <ambareeshbalaji@gmail.com>
 *  SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SelectionEditor.h"

#include "Selection.h"
#include "Geometry.h"
#include "settings.h"

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

using namespace Qt::StringLiterals;
using G = Geometry;
using Location = SelectionEditor::Location;

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
class SelectionEditorPrivate
{
public:
    SelectionEditorPrivate(SelectionEditor *q);

    SelectionEditor *const q;

    void updateHandlePositions();

    qreal dprRound(qreal value) const;

    void handleArrowKey(QKeyEvent *event);

    void setMouseCursor(QQuickItem *item, const QPointF &pos);
    Location mouseLocation(const QPointF &pos) const;

    void setDragLocation(Location location)
    {
        if (dragLocation == location) {
            return;
        }
        lastDragLocation = dragLocation;
        dragLocation = location;
        Q_EMIT q->dragLocationChanged();
    }

    bool validMagnifierLocation(Location location) const
    {
        return location != Location::None && location != Location::Inside;
    }

    void setShowMagnifier(bool show)
    {
        show = (show || Settings::showMagnifier() == Settings::ShowMagnifierAlways) //
            && Settings::showMagnifier() != Settings::ShowMagnifierNever //
            && validMagnifierLocation(magnifierLocation);
        if (showMagnifier == show) {
            return;
        }
        showMagnifier = show;
        Q_EMIT q->showMagnifierChanged();
    }

    void updateShowMagnifier()
    {
        setShowMagnifier(showMagnifier);
    }

    void setMagnifierLocation(Location location)
    {
        if (magnifierLocation == location) {
            return;
        }
        magnifierLocation = location;
        // if valid, move then show, else hide then move
        if (validMagnifierLocation(magnifierLocation)) {
            Q_EMIT q->magnifierLocationChanged();
            updateShowMagnifier();
        } else {
            setShowMagnifier(false);
            Q_EMIT q->magnifierLocationChanged();
        }
    }

    const std::unique_ptr<Selection> selection;

    QPointF startPos;
    QPointF initialTopLeft;
    Location dragLocation = Location::None;
    Location lastDragLocation = Location::None;
    qreal devicePixelRatio = 1;
    qreal devicePixel = 1;
    QPointF mousePos;
    bool showMagnifier = Settings::showMagnifier() == Settings::ShowMagnifierAlways;
    Location magnifierLocation = Location::FollowMouse;
    bool disableArrowKeys = false;
    QSet<Qt::Key> pressedKeys;
    QRectF screensRect;
    // Midpoints of handles
    QList<QPointF> handlePositions = QList<QPointF>{8};
    QRectF handlesRect;
    // Radius of handles is either handleRadiusMouse or handleRadiusTouch
    qreal handleRadius = s_handleRadiusMouse;
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

        offsetRight = translatedScreensRect.right() - right - handleRadius + devicePixel;
        offsetRight = (offsetRight >= 0) ? 0 : offsetRight;

        offsetBottom = translatedScreensRect.bottom() - bottom - handleRadius + devicePixel;
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

    // The current event should override any previously pressed keys
    bool leftArrow = key == Qt::Key_Left || (pressedKeys.contains(Qt::Key_Left) && !pressedKeys.contains(Qt::Key_Right));
    bool rightArrow = key == Qt::Key_Right || (pressedKeys.contains(Qt::Key_Right) && !pressedKeys.contains(Qt::Key_Left));
    bool upArrow = key == Qt::Key_Up || (pressedKeys.contains(Qt::Key_Up) && !pressedKeys.contains(Qt::Key_Down));
    bool downArrow = key == Qt::Key_Down || (pressedKeys.contains(Qt::Key_Down) && !pressedKeys.contains(Qt::Key_Up));

    const bool brMag = modifySize || selection->width() == 0.0 || selection->height() == 0.0;
    auto magLocation = Location::None;

    if (leftArrow) {
        magLocation = Location::Left;
        if (modifySize) {
            selectionRect = G::rectAdjustedVisually(selectionRect, 0, 0, -step, 0);
        } else {
            selectionRect.translate(-step, 0);
        }
    }
    if (rightArrow) {
        magLocation = Location::Right;
        if (modifySize) {
            selectionRect = G::rectAdjustedVisually(selectionRect, 0, 0, step, 0);
        } else {
            selectionRect.translate(step, 0);
        }
    }
    if (upArrow) {
        if (magLocation == Location::Left) {
            magLocation = Location::TopLeft;
        } else if (magLocation == Location::Right) {
            magLocation = Location::TopRight;
        } else {
            magLocation = Location::Top;
        }
        if (modifySize) {
            selectionRect = G::rectAdjustedVisually(selectionRect, 0, 0, 0, -step);
        } else {
            selectionRect.translate(0, -step);
        }
    }
    if (downArrow) {
        if (magLocation == Location::Left) {
            magLocation = Location::BottomLeft;
        } else if (magLocation == Location::Right) {
            magLocation = Location::BottomRight;
        } else {
            magLocation = Location::Bottom;
        }
        if (modifySize) {
            selectionRect = G::rectAdjustedVisually(selectionRect, 0, 0, 0, step);
        } else {
            selectionRect.translate(0, step);
        }
    }
    setMagnifierLocation(brMag ? Location::BottomRight : magLocation);
    selection->setRect(modifySize ? selectionRect : G::rectBounded(selectionRect, screensRect));
}

// TODO: change cursor with pointerhandlers in qml?
void SelectionEditorPrivate::setMouseCursor(QQuickItem *item, const QPointF &pos)
{
    const auto mouseState = mouseLocation(pos);
    if (mouseState == Location::Outside) {
        item->setCursor(Qt::CrossCursor);
    } else if (mouseState == Location::TopLeft || mouseState == Location::BottomRight) {
        item->setCursor(Qt::SizeFDiagCursor);
    } else if (mouseState == Location::TopRight || mouseState == Location::BottomLeft) {
        item->setCursor(Qt::SizeBDiagCursor);
    } else if (mouseState == Location::Top || mouseState == Location::Bottom) {
        item->setCursor(Qt::SizeVerCursor);
    } else if (mouseState == Location::Left || mouseState == Location::Right) {
        item->setCursor(Qt::SizeHorCursor);
    } else {
        item->setCursor(Qt::OpenHandCursor);
    }
}

SelectionEditor::Location SelectionEditorPrivate::mouseLocation(const QPointF &pos) const
{
    if (selection->isEmpty()) {
        return Location::Outside;
    }

    QRectF handleRect(-handleRadius, -handleRadius, handleRadius * 2, handleRadius * 2);
    if (G::ellipseContains(handleRect.translated(handlePositions[0]), pos)) {
        return Location::TopLeft;
    }
    if (G::ellipseContains(handleRect.translated(handlePositions[1]), pos)) {
        return Location::TopRight;
    }
    if (G::ellipseContains(handleRect.translated(handlePositions[2]), pos)) {
        return Location::BottomRight;
    }
    if (G::ellipseContains(handleRect.translated(handlePositions[3]), pos)) {
        return Location::BottomLeft;
    }
    if (G::ellipseContains(handleRect.translated(handlePositions[4]), pos)) {
        return Location::Top;
    }
    if (G::ellipseContains(handleRect.translated(handlePositions[5]), pos)) {
        return Location::Right;
    }
    if (G::ellipseContains(handleRect.translated(handlePositions[6]), pos)) {
        return Location::Bottom;
    }
    if (G::ellipseContains(handleRect.translated(handlePositions[7]), pos)) {
        return Location::Left;
    }

    const auto rect = selection->normalized();
    // Rectangle can be resized when border is dragged, if it's big enough
    if (rect.width() >= 100 && rect.height() >= 100) {
        if (rect.adjusted(0, -handleRadius, 0, -rect.height() + handleRadius).contains(pos)) {
            return Location::Top;
        }
        if (rect.adjusted(0, rect.height() - handleRadius, 0, handleRadius).contains(pos)) {
            return Location::Bottom;
        }
        if (rect.adjusted(-handleRadius, 0, -rect.width() + handleRadius, 0).contains(pos)) {
            return Location::Left;
        }
        if (rect.adjusted(rect.width() - handleRadius, 0, handleRadius, 0).contains(pos)) {
            return Location::Right;
        }
    }
    if (rect.contains(pos)) {
        return Location::Inside;
    }
    return Location::Outside;
}

// SelectionEditor =================================

SelectionEditor::SelectionEditor(QObject *parent)
    : QObject(parent)
    , d(new SelectionEditorPrivate(this))
{
    setObjectName(u"selectionEditor"_s);

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

QRectF SelectionEditor::screensRect() const
{
    return d->screensRect;
}

qreal SelectionEditor::screensWidth() const
{
    return d->screensRect.width();
}

qreal SelectionEditor::screensHeight() const
{
    return d->screensRect.height();
}

Location SelectionEditor::dragLocation() const
{
    return d->dragLocation;
}

QRectF SelectionEditor::handlesRect() const
{
    return d->handlesRect;
}

QPointF SelectionEditor::mousePosition() const
{
    return d->mousePos;
}

bool SelectionEditor::showMagnifier() const
{
    return d->showMagnifier;
}

Location SelectionEditor::magnifierLocation() const
{
    return d->magnifierLocation;
}

bool SelectionEditor::acceptSelection(ExportManager::Actions actions)
{
    if (d->screensRect.isEmpty()) {
        return false;
    }

    auto selectionRect = d->selection->normalized();
    if (Settings::rememberSelectionRect() == Settings::Always) {
        Settings::setSelectionRect(selectionRect.toAlignedRect());
    }

    if (selectionRect.isEmpty()) {
        selectionRect = d->screensRect;
    }

    Q_EMIT accepted(selectionRect, actions);
    return true;
}

void SelectionEditor::reset()
{
    qreal dpr = qGuiApp->devicePixelRatio();
    if (d->devicePixelRatio != dpr) {
        d->devicePixelRatio = dpr;
        d->devicePixel = 1 / dpr;
        Q_EMIT devicePixelRatioChanged();
    }

    auto rect = G::logicalScreensRect();
    if (d->screensRect != rect) {
        d->screensRect = rect;
        Q_EMIT screensRectChanged();
    }

    auto remember = Settings::rememberSelectionRect();
    if (remember == Settings::Never) {
        d->selection->setRect({});
    } else if (remember == Settings::Always) {
        auto selectionRect = Settings::selectionRect();
        if (selectionRect.width() < 0 || selectionRect.height() < 0) {
            selectionRect = {0, 0, 0, 0};
        }
        d->selection->setRect(selectionRect);
    }
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
    d->pressedKeys.insert(event->keyCombination().key());
    Q_UNUSED(item);
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
        d->setShowMagnifier(event->modifiers().testFlag(Qt::ShiftModifier));
        event->accept();
        break;
    case Qt::Key_Shift:
        d->setShowMagnifier(true);
        break;
    default:
        break;
    }
}

void SelectionEditor::keyReleaseEvent(QQuickItem *item, QKeyEvent *event)
{
    d->pressedKeys.remove(event->keyCombination().key());
    Q_UNUSED(item);
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
    case Qt::Key_Shift:
        d->setShowMagnifier(false);
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
    auto scenePosition = G::dprRound(event->scenePosition(), item->window()->devicePixelRatio());
    d->mousePos = mapSceneToLogicalGlobalPoint(scenePosition, item);
    Q_EMIT mousePositionChanged();
    d->setMouseCursor(item, d->mousePos);
    d->setShowMagnifier(event->modifiers().testFlag(Qt::ShiftModifier));
}

#include "SpectacleCore.h"
#include "ImageMetaData.h"
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
        auto scenePosition = G::dprRound(event->scenePosition(), item->window()->devicePixelRatio());
        d->mousePos = mapSceneToLogicalGlobalPoint(scenePosition, item);
        Q_EMIT mousePositionChanged();
        d->setDragLocation(d->mouseLocation(d->mousePos));
        d->setMagnifierLocation(d->dragLocation);
        d->disableArrowKeys = true;

        auto globalPos = item->mapToGlobal(event->position());
        qDebug() << "mouse pos:";
        if (globalPos != d->mousePos) {
            qDebug().nospace() << " [" << d->mousePos << ',' << globalPos << ']';
        } else {
            qDebug() << d->mousePos;
        }
        qDebug() << "window:" << item->window()->geometry();
        qDebug() << "screen:" << item->window()->screen()->geometry();
        qDebug() << "screensRect:" << d->screensRect;
        qDebug() << "canvasRect:" << SpectacleCore::instance()->annotationDocument()->canvasRect();
        qDebug() << "subgeometry:" << ImageMetaData::subGeometryList(SpectacleCore::instance()->annotationDocument()->baseImage());

        switch (d->dragLocation) {
        case Location::Outside:
            d->startPos = d->mousePos;
            break;
        case Location::Inside:
            d->startPos = d->mousePos;
            d->initialTopLeft = d->selection->rectF().topLeft();
            item->setCursor(Qt::ClosedHandCursor);
            break;
        case Location::Top:
        case Location::Left:
        case Location::TopLeft:
            d->startPos = d->selection->rectF().bottomRight();
            break;
        case Location::Bottom:
        case Location::Right:
        case Location::BottomRight:
            d->startPos = d->selection->rectF().topLeft();
            break;
        case Location::TopRight:
            d->startPos = d->selection->rectF().bottomLeft();
            break;
        case Location::BottomLeft:
            d->startPos = d->selection->rectF().topRight();
            break;
        default:
            break;
        }
    }
    event->accept();
}

void SelectionEditor::mouseMoveEvent(QQuickItem *item, QMouseEvent *event)
{
    if (!item->window() || !item->window()->screen()) {
        return;
    }

    auto scenePosition = G::dprRound(event->scenePosition(), item->window()->devicePixelRatio());
    d->mousePos = mapSceneToLogicalGlobalPoint(scenePosition, item);
    Q_EMIT mousePositionChanged();
    d->setMagnifierLocation(d->dragLocation);
    switch (d->dragLocation) {
    case Location::None: {
        d->setMouseCursor(item, d->mousePos);
        break;
    }
    case Location::TopLeft:
    case Location::TopRight:
    case Location::BottomRight:
    case Location::BottomLeft: {
        const bool afterX = d->mousePos.x() >= d->startPos.x();
        const bool afterY = d->mousePos.y() >= d->startPos.y();
        d->selection->setRect(afterX ? d->startPos.x() : d->mousePos.x(),
                              afterY ? d->startPos.y() : d->mousePos.y(),
                              qAbs(d->mousePos.x() - d->startPos.x()) + (afterX ? d->devicePixel : 0),
                              qAbs(d->mousePos.y() - d->startPos.y()) + (afterY ? d->devicePixel : 0));
        break;
    }
    case Location::Outside: {
        d->selection->setRect(qMin(d->mousePos.x(), d->startPos.x()),
                              qMin(d->mousePos.y(), d->startPos.y()),
                              qAbs(d->mousePos.x() - d->startPos.x()) + d->devicePixel,
                              qAbs(d->mousePos.y() - d->startPos.y()) + d->devicePixel);
        break;
    }
    case Location::Top:
    case Location::Bottom: {
        const bool afterY = d->mousePos.y() >= d->startPos.y();
        d->selection->setRect(d->selection->x(),
                              afterY ? d->startPos.y() : d->mousePos.y(),
                              d->selection->width(),
                              qAbs(d->mousePos.y() - d->startPos.y()) + (afterY ? d->devicePixel : 0));
        break;
    }
    case Location::Right:
    case Location::Left: {
        const bool afterX = d->mousePos.x() >= d->startPos.x();
        d->selection->setRect(afterX ? d->startPos.x() : d->mousePos.x(),
                              d->selection->y(),
                              qAbs(d->mousePos.x() - d->startPos.x()) + (afterX ? d->devicePixel : 0),
                              d->selection->height());
        break;
    }
    case Location::Inside: {
        // We use some math here to figure out if the diff with which we move the rectangle
        QRectF newRect(d->mousePos - d->startPos + d->initialTopLeft, d->selection->sizeF());
        d->selection->setRect(G::rectBounded(newRect, d->screensRect));
        break;
    }
    default:
        break;
    }

    event->accept();
}

void SelectionEditor::mouseReleaseEvent(QQuickItem *item, QMouseEvent *event)
{
    switch (event->button()) {
    case Qt::LeftButton:
    case Qt::RightButton:
        if (d->dragLocation == Location::Outside && Settings::useReleaseToCapture()) {
            acceptSelection();
        } else {
            d->disableArrowKeys = false;
            if (d->dragLocation == Location::Inside) {
                item->setCursor(Qt::OpenHandCursor);
            }
        }
        break;
    default:
        break;
    }
    d->setDragLocation(Location::None);
    d->setMagnifierLocation(Location::FollowMouse);
    event->accept();
}

void SelectionEditor::mouseDoubleClickEvent(QQuickItem *item, QMouseEvent *event)
{
    Q_UNUSED(item)
    if (event->button() == Qt::LeftButton && (d->selection->contains(d->mousePos) || d->selection->isEmpty())) {
        acceptSelection();
    }
    d->setMagnifierLocation(Location::FollowMouse);
    event->accept();
}

#include <moc_SelectionEditor.cpp>
