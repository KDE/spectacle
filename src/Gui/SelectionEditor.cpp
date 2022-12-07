/*
 *  SPDX-FileCopyrightText: 2018 Ambareesh "Amby" Balaji <ambareeshbalaji@gmail.com>
 *  SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SelectionEditor.h"

#include "Annotations/AnnotationDocument.h"
#include "SpectacleCore.h"
#include "settings.h"
#include "spectacle_gui_debug.h"

#include <KLocalizedString>
#include <KWayland/Client/plasmashell.h>
#include <KWindowSystem>

#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPixmapCache>
#include <QQuickWindow>
#include <QScreen>
#include <QtMath>
#include <qnamespace.h>

class SelectionEditorSingleton
{
public:
    SelectionEditor self;
};

Q_GLOBAL_STATIC(SelectionEditorSingleton, privateSelectionEditorSelf)

static constexpr int s_handleRadiusMouse = 9;
static constexpr int s_handleRadiusTouch = 12;
static constexpr qreal s_increaseDragAreaFactor = 2.0;
static constexpr int s_minSpacingBetweenHandles = 20;
static constexpr int s_borderDragAreaSize = 10;

static constexpr int s_magnifierLargeStep = 15;

static constexpr inline bool isPointInsideCircle(const QPointF &circleCenter, qreal radius, const QPointF &point) noexcept
{
    return (std::pow(point.x() - circleCenter.x(), 2) + std::pow(point.y() - circleCenter.y(), 2) <= std::pow(radius, 2)) ? true : false;
}

static constexpr inline bool inRange(qreal low, qreal high, qreal value) noexcept
{
    return value >= low && value <= high;
}

static constexpr inline bool withinThreshold(qreal offset, qreal threshold) noexcept
{
    return std::fabs(offset) <= threshold;
}

// SelectionEditorPrivate =====================

using MouseLocation = SelectionEditor::MouseLocation;

class SelectionEditorPrivate
{
public:
    SelectionEditorPrivate(SelectionEditor *q);
    ~SelectionEditorPrivate();

    SelectionEditor *const q;

    void updateDevicePixelRatio();
    void updateHandlePositions();

    int boundsLeft(int newTopLeftX, const bool mouse = true);
    int boundsRight(int newTopLeftX, const bool mouse = true);
    int boundsUp(int newTopLeftY, const bool mouse = true);
    int boundsDown(int newTopLeftY, const bool mouse = true);

    void handleArrowKey(QKeyEvent *event);

    void setMouseCursor(QQuickItem *item, const QPointF &pos);
    MouseLocation mouseLocation(const QPointF &pos) const;

    Selection *const selection;

    QPointF startPos;
    QPointF initialTopLeft;
    MouseLocation dragLocation = MouseLocation::None;
    QVector<ScreenImage> screenImages;
    QPixmap pixmap;
    qreal devicePixelRatio = 1;
    qreal devicePixelRatioI = 1;
    QPointF mousePos;
    bool magnifierAllowed = false;
    bool toggleMagnifier = false;
    bool disableArrowKeys = false;
    QRect screensRect;
    // Midpoints of handles
    QVector<QPointF> handlePositions = QVector<QPointF>{8};
    QRectF handlesRect;
    // Radius of handles is either handleRadiusMouse or handleRadiusTouch
    qreal handleRadius = s_handleRadiusMouse;
    qreal penWidth;
    qreal penOffset;
};

SelectionEditorPrivate::SelectionEditorPrivate(SelectionEditor *q)
    : q(q)
    , selection(new Selection(q))
{
}

SelectionEditorPrivate::~SelectionEditorPrivate() noexcept
{
    delete selection;
}

void SelectionEditorPrivate::updateDevicePixelRatio()
{
    if (KWindowSystem::isPlatformWayland()) {
        devicePixelRatio = 1.0;
    } else {
        devicePixelRatio = qApp->devicePixelRatio();
    }

    devicePixelRatioI = 1.0 / devicePixelRatio;
    penWidth = q->dprRound(1.0);
    penOffset = penWidth / 2.0;
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
        const QRect translatedScreensRect = screensRect.translated(-screensRect.topLeft());

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

int SelectionEditorPrivate::boundsLeft(int newTopLeftX, const bool mouse)
{
    if (newTopLeftX < 0) {
        if (mouse) {
            // tweak startPos to prevent rectangle from getting stuck
            startPos.setX(startPos.x() + newTopLeftX * devicePixelRatioI);
        }
        newTopLeftX = 0;
    }

    return newTopLeftX;
}

int SelectionEditorPrivate::boundsRight(int newTopLeftX, const bool mouse)
{
    // the max X coordinate of the top left point
    const int realMaxX = qRound((q->width() - selection->width()) * devicePixelRatio);
    const int xOffset = newTopLeftX - realMaxX;
    if (xOffset > 0) {
        if (mouse) {
            startPos.setX(startPos.x() + xOffset * devicePixelRatioI);
        }
        newTopLeftX = realMaxX;
    }

    return newTopLeftX;
}

int SelectionEditorPrivate::boundsUp(int newTopLeftY, const bool mouse)
{
    if (newTopLeftY < 0) {
        if (mouse) {
            startPos.setY(startPos.y() + newTopLeftY * devicePixelRatioI);
        }
        newTopLeftY = 0;
    }

    return newTopLeftY;
}

int SelectionEditorPrivate::boundsDown(int newTopLeftY, const bool mouse)
{
    // the max Y coordinate of the top left point
    const int realMaxY = qRound((q->height() - selection->height()) * devicePixelRatio);
    const int yOffset = newTopLeftY - realMaxY;
    if (yOffset > 0) {
        if (mouse) {
            startPos.setY(startPos.y() + yOffset * devicePixelRatioI);
        }
        newTopLeftY = realMaxY;
    }

    return newTopLeftY;
}

void SelectionEditorPrivate::handleArrowKey(QKeyEvent *event)
{
    if (disableArrowKeys) {
        return;
    }

    const auto key = static_cast<Qt::Key>(event->key());
    const auto modifiers = event->modifiers();
    const qreal step = (modifiers & Qt::ShiftModifier ? 1 : s_magnifierLargeStep);
    QRectF selectionRect = selection->rectF();

    if (key == Qt::Key_Left) {
        const int newPos = boundsLeft(qRound(selectionRect.left() * devicePixelRatio - step), false);
        if (modifiers & Qt::AltModifier) {
            selectionRect.setRight(devicePixelRatioI * newPos + selectionRect.width());
            selectionRect = selectionRect.normalized();
        } else {
            selectionRect.moveLeft(devicePixelRatioI * newPos);
        }
    } else if (key == Qt::Key_Right) {
        const int newPos = boundsRight(qRound(selectionRect.left() * devicePixelRatio + step), false);
        if (modifiers & Qt::AltModifier) {
            selectionRect.setRight(devicePixelRatioI * newPos + selectionRect.width());
        } else {
            selectionRect.moveLeft(devicePixelRatioI * newPos);
        }
    } else if (key == Qt::Key_Up) {
        const int newPos = boundsUp(qRound(selectionRect.top() * devicePixelRatio - step), false);
        if (modifiers & Qt::AltModifier) {
            selectionRect.setBottom(devicePixelRatioI * newPos + selectionRect.height());
            selectionRect = selectionRect.normalized();
        } else {
            selectionRect.moveTop(devicePixelRatioI * newPos);
        }
    } else if (key == Qt::Key_Down) {
        const int newPos = boundsDown(qRound(selectionRect.top() * devicePixelRatio + step), false);
        if (modifiers & Qt::AltModifier) {
            selectionRect.setBottom(devicePixelRatioI * newPos + selectionRect.height());
        } else {
            selectionRect.moveTop(devicePixelRatioI * newPos);
        }
    }
    selection->setRect(selectionRect);
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
    if (isPointInsideCircle(handlePositions[0], handleRadius * s_increaseDragAreaFactor, pos)) {
        return MouseLocation::TopLeft;
    }
    if (isPointInsideCircle(handlePositions[1], handleRadius * s_increaseDragAreaFactor, pos)) {
        return MouseLocation::TopRight;
    }
    if (isPointInsideCircle(handlePositions[2], handleRadius * s_increaseDragAreaFactor, pos)) {
        return MouseLocation::BottomRight;
    }
    if (isPointInsideCircle(handlePositions[3], handleRadius * s_increaseDragAreaFactor, pos)) {
        return MouseLocation::BottomLeft;
    }
    if (isPointInsideCircle(handlePositions[4], handleRadius * s_increaseDragAreaFactor, pos)) {
        return MouseLocation::Top;
    }
    if (isPointInsideCircle(handlePositions[5], handleRadius * s_increaseDragAreaFactor, pos)) {
        return MouseLocation::Right;
    }
    if (isPointInsideCircle(handlePositions[6], handleRadius * s_increaseDragAreaFactor, pos)) {
        return MouseLocation::Bottom;
    }
    if (isPointInsideCircle(handlePositions[7], handleRadius * s_increaseDragAreaFactor, pos)) {
        return MouseLocation::Left;
    }

    // Rectangle can be resized when border is dragged, if it's big enough
    if (selection->width() >= 100 && selection->height() >= 100) {
        if (inRange(selection->x(), selection->x() + selection->width(), pos.x())) {
            if (withinThreshold(pos.y() - selection->y(), s_borderDragAreaSize)) {
                return MouseLocation::Top;
            }
            if (withinThreshold(pos.y() - selection->y() - selection->height(), s_borderDragAreaSize)) {
                return MouseLocation::Bottom;
            }
        }
        if (inRange(selection->y(), selection->y() + selection->height(), pos.y())) {
            if (withinThreshold(pos.x() - selection->x(), s_borderDragAreaSize)) {
                return MouseLocation::Left;
            }
            if (withinThreshold(pos.x() - selection->x() - selection->width(), s_borderDragAreaSize)) {
                return MouseLocation::Right;
            }
        }
    }
    if (selection->contains(pos.toPoint())) {
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
    connect(SpectacleCore::instance(), &SpectacleCore::captureWindowAdded, this, [this](CaptureWindow *window) {
        connect(window, &CaptureWindow::devicePixelRatioChanged, this, &SelectionEditor::devicePixelRatioChanged);
        window->rootObject()->installEventFilter(this);
    });
    connect(SpectacleCore::instance(), &SpectacleCore::captureWindowRemoved, this, [this](CaptureWindow *window) {
        window->rootObject()->removeEventFilter(this);
    });

    d->updateDevicePixelRatio();

    connect(d->selection, &Selection::rectChanged, this, [this](){
        d->updateHandlePositions();
    });

    if (Settings::rememberLastRectangularRegion() == Settings::Always) {
        QRectF cropRegion = Settings::cropRegion();
        if (!cropRegion.isEmpty()) {
            cropRegion.setRect(cropRegion.x() * d->devicePixelRatioI,
                               cropRegion.y() * d->devicePixelRatioI,
                               cropRegion.width() * d->devicePixelRatioI,
                               cropRegion.height() * d->devicePixelRatioI);
            d->selection->setRect(cropRegion.intersected(QRectF(d->screensRect.x(), d->screensRect.y(), d->screensRect.width(), d->screensRect.height())));
        }
    }
}

SelectionEditor::~SelectionEditor() noexcept
{
    delete d;
}

SelectionEditor *SelectionEditor::instance()
{
    return &privateSelectionEditorSelf()->self;
}

Selection *SelectionEditor::selection() const
{
    return d->selection;
}

qreal SelectionEditor::devicePixelRatio() const
{
    return d->devicePixelRatio;
}

QRect SelectionEditor::screensRect() const
{
    return d->screensRect;
}

int SelectionEditor::width() const
{
    return d->screensRect.width();
}

int SelectionEditor::height() const
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

qreal SelectionEditor::dprRound(qreal value, qreal dpr) const
{
    return std::round(value * dpr) / dpr;
}

QImage SelectionEditor::imageForScreen(QScreen *screen)
{
    for (const auto &si : qAsConst(d->screenImages)) {
        if (si.screen == screen) {
            return si.image;
        }
    }
    return QImage();
}

QImage SelectionEditor::imageForScreenName(const QString &name)
{
    for (const auto &si : qAsConst(d->screenImages)) {
        if (si.screen->name() == name) {
            return si.image;
        }
    }
    return QImage();
}

qreal SelectionEditor::dprRound(qreal value) const
{
    return dprRound(value, d->devicePixelRatio);
}

void SelectionEditor::setScreenImages(const QVector<ScreenImage> &screenImages)
{
    QVector<QPoint> translatedPoints;
    QRect screensRect;
    for (int i = 0; i < screenImages.length(); ++i) {
        const QScreen *screen = screenImages[i].screen;
        const QImage &image = screenImages[i].image;
        const qreal dpr = screenImages[i].devicePixelRatio;

        QRect virtualScreenRect;
        if (KWindowSystem::isPlatformX11()) {
            virtualScreenRect = QRect(screen->geometry().topLeft(), image.size());
        } else {
            // `QSize / qreal` divides the int width and int height by the qreal factor,
            // then rounds the results to the nearest int.
            virtualScreenRect = QRect(screen->geometry().topLeft(), image.size() / dpr);
        }
        screensRect = screensRect.united(virtualScreenRect);

        translatedPoints.append(screen->geometry().topLeft());
    }

    d->screenImages = screenImages;

    // compute coordinates after scaling
    for (int i = 0; i < screenImages.length(); ++i) {
        const QScreen *screen = screenImages[i].screen;
        const QImage &image = screenImages[i].image;
        const QPoint &p = screen->geometry().topLeft();
        const QSize &size = image.size();
        const double dpr = screenImages[i].devicePixelRatio;
        if (!qFuzzyCompare(dpr, 1.0)) {
            // must update all coordinates of next rects
            int newWidth = size.width();
            int newHeight = size.height();

            int deltaX = newWidth - (size.width());
            int deltaY = newHeight - (size.height());

            // for the next size
            for (int i2 = i; i2 < screenImages.length(); ++i2) {
                auto point = screenImages[i2].screen->geometry().topLeft();

                if (point.x() >= newWidth + p.x() - deltaX) {
                    translatedPoints[i2].setX(translatedPoints[i2].x() + deltaX);
                }
                if (point.y() >= newHeight + p.y() - deltaY) {
                    translatedPoints[i2].setY(translatedPoints[i2].y() + deltaY);
                }
            }
        }
    }

    d->pixmap = QPixmap(screensRect.size());
    QPainter painter(&d->pixmap);
    // Don't enable SmoothPixmapTransform, we want crisp graphics.
    for (int i = 0; i < screenImages.length(); ++i) {
        // Geometry can have negative coordinates,
        // so it is necessary to subtract the upper left point,
        // because coordinates on the widget are counted from 0.
        painter.drawImage(translatedPoints[i] - screensRect.topLeft(), screenImages[i].image);
    }

    if (d->screensRect != screensRect) {
        d->screensRect = screensRect;
        Q_EMIT screensRectChanged();
    }

    Q_EMIT screenImagesChanged();
}

QVector<ScreenImage> SelectionEditor::screenImages() const
{
    return d->screenImages;
}

bool SelectionEditor::acceptSelection()
{
    if (d->selection->isEmpty() || d->screenImages.isEmpty() /*TODO || !isVisible()*/) {
        return false;
    }
    QRect scaledCropRegion(d->selection->alignedRect(d->devicePixelRatio));
    if (Settings::rememberLastRectangularRegion() == Settings::Always) {
        Settings::setCropRegion(scaledCropRegion);
    }

    auto spectacleCore = SpectacleCore::instance();
    spectacleCore->annotationDocument()->cropCanvas(d->selection->alignedRect(1));

    if (KWindowSystem::isPlatformX11()) {
        d->pixmap.setDevicePixelRatio(qGuiApp->devicePixelRatio());
        if (scaledCropRegion.size() != d->pixmap.size()) {
            Q_EMIT spectacleCore->grabDone(d->pixmap.copy(scaledCropRegion));
        } else {
            Q_EMIT spectacleCore->grabDone(d->pixmap);
        }
    } else { // Wayland case
        // QGuiApplication::devicePixelRatio() is calculated by getting the highest screen DPI
        qreal maxDpr = qGuiApp->devicePixelRatio();

        QRect selectionRect = d->selection->alignedRect();
        QSize selectionSize = selectionRect.size();
        QPixmap output(selectionSize * maxDpr);
        QPainter painter(&output);
        // Don't enable SmoothPixmapTransform, we want crisp graphics

        for (auto it = d->screenImages.constBegin(); it != d->screenImages.constEnd(); ++it) {
            const auto screen = it->screen;
            const auto &screenRect = screen->geometry();

            if (selectionRect.intersects(screenRect)) {
                const QPoint pos = screenRect.topLeft();
                const qreal dpr = it->devicePixelRatio;

                QRect intersected = screenRect.intersected(selectionRect);

                // converts to screen size & position
                QRect pixelOnScreenIntersected;
                pixelOnScreenIntersected.moveTopLeft((intersected.topLeft() - pos) * dpr);
                pixelOnScreenIntersected.setWidth(intersected.width() * dpr);
                pixelOnScreenIntersected.setHeight(intersected.height() * dpr);

                QPixmap screenOutput = QPixmap::fromImage(it->image.copy(pixelOnScreenIntersected));

                // FIXME: this doesn't seem correct
                if (intersected.size() == selectionSize) {
                    // short path when single screen
                    // keep native screen resolution
                    // we need to set the pixmap dpr to be able to properly align with annotations
                    screenOutput.setDevicePixelRatio(dpr);
                    Q_EMIT spectacleCore->grabDone(screenOutput);
                    return true;
                }

                // upscale the image according to max screen dpr, to keep the image not distorted
                output.setDevicePixelRatio(maxDpr);
                const auto dprI = maxDpr / dpr;
                QBrush brush(screenOutput);
                brush.setTransform(QTransform::fromScale(dprI, dprI));
                intersected.moveTopLeft((intersected.topLeft() - selectionRect.topLeft()) * maxDpr);
                intersected.setSize(intersected.size() * maxDpr);
                painter.setBrushOrigin(intersected.topLeft());
                painter.fillRect(intersected, brush);
            }
        }

        Q_EMIT spectacleCore->grabDone(output);
    }

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
    d->mousePos = event->posF() + item->window()->screen()->geometry().topLeft();
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

    if (event->button() & Qt::LeftButton) {
        item->setFocus(true);
        const bool wasMagnifierAllowed = d->magnifierAllowed;
        d->mousePos = event->localPos() + item->window()->screen()->geometry().topLeft();
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

    d->mousePos = event->localPos() + item->window()->screen()->geometry().topLeft();
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
                              qAbs(d->mousePos.x() - d->startPos.x()) + (afterX ? d->devicePixelRatioI : 0),
                              qAbs(d->mousePos.y() - d->startPos.y()) + (afterY ? d->devicePixelRatioI : 0));
        break;
    }
    case MouseLocation::Outside: {
        d->selection->setRect(qMin(d->mousePos.x(), d->startPos.x()),
                              qMin(d->mousePos.y(), d->startPos.y()),
                              qAbs(d->mousePos.x() - d->startPos.x()) + d->devicePixelRatioI,
                              qAbs(d->mousePos.y() - d->startPos.y()) + d->devicePixelRatioI);
        break;
    }
    case MouseLocation::Top:
    case MouseLocation::Bottom: {
        const bool afterY = d->mousePos.y() >= d->startPos.y();
        d->selection->setRect(d->selection->x(),
                              afterY ? d->startPos.y() : d->mousePos.y(),
                              d->selection->width(),
                              qAbs(d->mousePos.y() - d->startPos.y()) + (afterY ? d->devicePixelRatioI : 0));
        break;
    }
    case MouseLocation::Right:
    case MouseLocation::Left: {
        const bool afterX = d->mousePos.x() >= d->startPos.x();
        d->selection->setRect(afterX ? d->startPos.x() : d->mousePos.x(),
                              d->selection->y(),
                              qAbs(d->mousePos.x() - d->startPos.x()) + (afterX ? d->devicePixelRatioI : 0),
                              d->selection->height());
        break;
    }
    case MouseLocation::Inside: {
        d->magnifierAllowed = false;
        // We use some math here to figure out if the diff with which we
        // move the rectangle with moves it out of bounds,
        // in which case we adjust the diff to not let that happen

        // new top left point of the rectangle
        QPointF newTopLeft = (d->mousePos - d->startPos + d->initialTopLeft) * d->devicePixelRatio;

        const QRectF newRect(newTopLeft, d->selection->sizeF() * d->devicePixelRatio);

        const QRectF translatedScreensRect = d->screensRect.translated(-d->screensRect.topLeft());
        if (!translatedScreensRect.contains(newRect)) {
            // Keep the item inside the scene screen region bounding rect.
            newTopLeft.setX(qMin(translatedScreensRect.right() - newRect.width(), qMax(newTopLeft.x(), translatedScreensRect.left())));
            newTopLeft.setY(qMin(translatedScreensRect.bottom() - newRect.height(), qMax(newTopLeft.y(), translatedScreensRect.top())));
        }

        d->selection->moveTo(newTopLeft * d->devicePixelRatioI);
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
        if (d->dragLocation == MouseLocation::Outside && Settings::useReleaseToCapture()) {
            acceptSelection();
            return;
        }
        d->disableArrowKeys = false;
        if (d->dragLocation == MouseLocation::Inside) {
            item->setCursor(Qt::OpenHandCursor);
        }
        break;
    case Qt::RightButton:
        d->selection->setRect(QRect());
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
