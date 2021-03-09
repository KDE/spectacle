/*
 *  SPDX-FileCopyrightText: 2018 Ambareesh "Amby" Balaji <ambareeshbalaji@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KLocalizedString>
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>
#include <KWindowSystem>
#include <QGuiApplication>
#include <QScreen>
#include <QtCore/qmath.h>
#include <QPainterPath>
#include <QX11Info>

#include "QuickEditor.h"
#include "settings.h"

const int QuickEditor::handleRadiusMouse = 9;
const int QuickEditor::handleRadiusTouch = 12;
const qreal QuickEditor::increaseDragAreaFactor = 2.0;
const int QuickEditor::minSpacingBetweenHandles = 20;
const int QuickEditor::borderDragAreaSize = 10;

const int QuickEditor::selectionSizeThreshold = 100;

const int QuickEditor::selectionBoxPaddingX = 5;
const int QuickEditor::selectionBoxPaddingY = 4;
const int QuickEditor::selectionBoxMarginY = 5;

bool QuickEditor::bottomHelpTextPrepared = false;
const int QuickEditor::bottomHelpBoxPaddingX = 12;
const int QuickEditor::bottomHelpBoxPaddingY = 8;
const int QuickEditor::bottomHelpBoxPairSpacing = 6;
const int QuickEditor::bottomHelpBoxMarginBottom = 5;
const int QuickEditor::midHelpTextFontSize = 12;

const int QuickEditor::magnifierLargeStep = 15;

const int QuickEditor::magZoom = 5;
const int QuickEditor::magPixels = 16;
const int QuickEditor::magOffset = 32;

QuickEditor::QuickEditor(const QMap<ComparableQPoint, QImage> &images, KWayland::Client::PlasmaShell *plasmashell, QWidget *parent) :
    QWidget(parent),
    mMaskColor(QColor::fromRgbF(0, 0, 0, 0.15)),
    mStrokeColor(palette().highlight().color()),
    mCrossColor(QColor::fromRgbF(mStrokeColor.redF(), mStrokeColor.greenF(), mStrokeColor.blueF(), 0.7)),
    mLabelBackgroundColor(QColor::fromRgbF(
        palette().light().color().redF(),
        palette().light().color().greenF(),
        palette().light().color().blueF(),
        0.85
    )),
    mLabelForegroundColor(palette().windowText().color()),
    mMidHelpText(i18n("Click and drag to draw a selection rectangle,\nor press Esc to quit")),
    mMidHelpTextFont(font()),
    mBottomHelpTextFont(font()),
    mBottomHelpGridLeftWidth(0),
    mMouseDragState(MouseState::None),
    mImages(images),
    mMagnifierAllowed(false),
    mShowMagnifier(Settings::showMagnifier()),
    mToggleMagnifier(false),
    mReleaseToCapture(Settings::useReleaseToCapture()),
    mRememberRegion(Settings::alwaysRememberRegion() || Settings::rememberLastRectangularRegion()),
    mDisableArrowKeys(false),
    mPrimaryScreenGeo(QGuiApplication::primaryScreen()->geometry()),
    mbottomHelpLength(bottomHelpMaxLength),
    mHandleRadius(handleRadiusMouse)
{
    if (Settings::useLightMaskColour()) {
        mMaskColor = QColor(255, 255, 255, 100);
    }

    setMouseTracking(true);
    setAttribute(Qt::WA_StaticContents);
    setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::Popup | Qt::WindowStaysOnTopHint);

    devicePixelRatio = plasmashell ? 1.0 : devicePixelRatioF();
    devicePixelRatioI = 1.0 / devicePixelRatio;

    int width = 0, height = 0;
    for (auto it = mImages.constBegin(); it != mImages.constEnd(); it ++) {
        width = qMax(width, it.value().width() + it.key().x());
        height = qMax(height, it.value().height() + it.key().y());
    }

    mPixmap = QPixmap(width, height);
    mScreenRegion = QRegion();

    const QList<QScreen*> screens = QGuiApplication::screens();
    QMap<ComparableQPoint, QPair<qreal, QSize>> input;
    for (auto it = mImages.begin(); it != mImages.end(); ++it) {
        const auto pos = it.key();
        const QImage &screenImage = it.value();
        auto item = std::find_if(screens.constBegin(), screens.constEnd(),
                                      [pos] (const QScreen* screen){
            return screen->geometry().topLeft() == pos;
        });
        const QScreen* screen = *item;
        input.insert(pos, QPair<qreal, QSize>(screenImage.width() / static_cast<qreal>(screen->size().width()), screenImage.size()));
    }
    const auto pointsTranslationMap = computeCoordinatesAfterScaling(input);
    QPainter painter(&mPixmap);
    for (auto it = mImages.constBegin(); it != mImages.constEnd(); it ++) {
        painter.drawImage(pointsTranslationMap.value(it.key()), it.value());
    }
    painter.end();

    if (KWindowSystem::isPlatformX11()) {
        // Even though we want the quick editor window to be placed at (0, 0) in the native
        // pixels, we cannot really specify a window position of (0, 0) if HiDPI support is on.
        //
        // The main reason for that is that Qt will scale the window position relative to the
        // upper left corner of the screen where the quick editor is on in order to perform
        // a conversion from the device-independent coordinates to the native pixels.
        //
        // Since (0, 0) in the device-independent pixels may not correspond to (0, 0) in the
        // native pixels, we use XCB API to place the quick editor window at (0, 0).

        uint16_t mask = 0;

        mask |= XCB_CONFIG_WINDOW_X;
        mask |= XCB_CONFIG_WINDOW_Y;

        const uint32_t values[] = {
            /* x */ 0,
            /* y */ 0,
        };

        xcb_configure_window(QX11Info::connection(), winId(), mask, values);
        resize(width, height);
    } else {
        setGeometry(0, 0, width, height);
    }

    // TODO This is a hack until a better interface is available
    if (plasmashell) {
        using namespace KWayland::Client;
        winId();
        auto surface = Surface::fromWindow(windowHandle());
        if (surface) {
            PlasmaShellSurface *plasmashellSurface = plasmashell->createSurface(surface, this);
            plasmashellSurface->setRole(PlasmaShellSurface::Role::Panel);
            plasmashellSurface->setPanelTakesFocus(true);
            plasmashellSurface->setPosition(geometry().topLeft());
        }
    }
    if (Settings::rememberLastRectangularRegion() || Settings::alwaysRememberRegion()) {
        auto savedRect = Settings::cropRegion();
        QRect cropRegion = QRect(savedRect[0], savedRect[1], savedRect[2], savedRect[3]);
        if (!cropRegion.isEmpty()) {
            mSelection = QRect(
                cropRegion.x() * devicePixelRatioI,
                cropRegion.y() * devicePixelRatioI,
                cropRegion.width() * devicePixelRatioI,
                cropRegion.height() * devicePixelRatioI
            ).intersected(rect());
        }
        setMouseCursor(QCursor::pos());
    } else {
        setCursor(Qt::CrossCursor);
    }

    setBottomHelpText();
    mMidHelpTextFont.setPointSize(midHelpTextFontSize);
    if (!bottomHelpTextPrepared) {
        bottomHelpTextPrepared = true;
        const auto prepare = [this](QStaticText& item) {
            item.prepare(QTransform(), mBottomHelpTextFont);
            item.setPerformanceHint(QStaticText::AggressiveCaching);
        };
        for (auto& pair : mBottomHelpText) {
            prepare(pair.first);
            for (auto &item : pair.second) {
                prepare(item);
            }
        }
    }
    layoutBottomHelpText();

    preparePaint();
    update();
}

void QuickEditor::acceptSelection()
{
    if (!mSelection.isEmpty()) {

        QRect scaledCropRegion = QRect(
            qRound(mSelection.x() * devicePixelRatio),
            qRound(mSelection.y() * devicePixelRatio),
            qRound(mSelection.width() * devicePixelRatio),
            qRound(mSelection.height() * devicePixelRatio)
        );
        Settings::setCropRegion({scaledCropRegion.x(), scaledCropRegion.y(), scaledCropRegion.width(), scaledCropRegion.height()});

        if (KWindowSystem::isPlatformX11()) {
            emit grabDone(mPixmap.copy(scaledCropRegion));

        } else {
            // Wayland case
            qreal maxDpr = 1.0;
            for (const QScreen *screen: QGuiApplication::screens()) {
                if (screen->devicePixelRatio() > maxDpr) {
                    maxDpr = screen->devicePixelRatio();
                }
            }

            QPixmap output(mSelection.size() * maxDpr);
            QPainter painter(&output);
            QRect intersected;
            QPixmap screenOutput;

            for (auto it = mRectToDpr.constBegin(); it != mRectToDpr.constEnd(); ++it)
            {
                const auto screenRect = (*it).first;

                if (mSelection.intersects(screenRect)) {
                    const auto &pos = screenRect.topLeft();
                    const qreal &dpr = (*it).second;

                    intersected = screenRect.intersected(mSelection);

                    // converts to screen size & position
                    QRect pixelOnScreenIntersected;
                    pixelOnScreenIntersected.moveTopLeft((intersected.topLeft() - pos) * dpr);
                    pixelOnScreenIntersected.setWidth(intersected.width() * dpr);
                    pixelOnScreenIntersected.setHeight(intersected.height() * dpr);

                    screenOutput = QPixmap::fromImage(mImages.value(pos).copy(pixelOnScreenIntersected));

                    if (intersected.size() == mSelection.size()) {
                        painter.end();

                        // short path when single screen
                        // keep native screen resolution
                        emit grabDone(screenOutput);
                        return;

                    } else {

                        // upscale the image according to max screen dpr, to keep the image not distorted
                        const auto dprI = maxDpr / dpr;
                        QBrush brush(screenOutput);
                        brush.setTransform(QTransform().scale(dprI, dprI));
                        intersected.moveTopLeft((intersected.topLeft() - mSelection.topLeft()) * maxDpr);
                        intersected.setSize(intersected.size() * maxDpr);
                        painter.setBrushOrigin(intersected.topLeft());
                        painter.fillRect(intersected, brush);
                    }
                }
            }
            painter.end();

            emit grabDone(output);
        }
    }
}

void QuickEditor::keyPressEvent(QKeyEvent* event)
{
    const auto modifiers = event->modifiers();
    const bool shiftPressed = modifiers & Qt::ShiftModifier;
    if (shiftPressed) {
        mToggleMagnifier = true;
    }
    switch(event->key()) {
    case Qt::Key_Escape:
        emit grabCancelled();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        acceptSelection();
        break;
    case Qt::Key_Up: {
        if(mDisableArrowKeys) {
            update();
            break;
        }
        const qreal step = (shiftPressed ? 1 : magnifierLargeStep);
        const int newPos = boundsUp(qRound(mSelection.top() * devicePixelRatio - step), false);
        if (modifiers & Qt::AltModifier) {
            mSelection.setBottom(devicePixelRatioI * newPos + mSelection.height());
            mSelection = mSelection.normalized();
        } else {
            mSelection.moveTop(devicePixelRatioI * newPos);
        }
        update();
        break;
    }
    case Qt::Key_Right: {
        if(mDisableArrowKeys) {
            update();
            break;
        }
        const qreal step = (shiftPressed ? 1 : magnifierLargeStep);
        const int newPos = boundsRight(qRound(mSelection.left() * devicePixelRatio + step), false);
        if (modifiers & Qt::AltModifier) {
            mSelection.setRight(devicePixelRatioI * newPos + mSelection.width());
        } else {
            mSelection.moveLeft(devicePixelRatioI * newPos);
        }
        update();
        break;
    }
    case Qt::Key_Down: {
        if(mDisableArrowKeys) {
            update();
            break;
        }
        const qreal step = (shiftPressed ? 1 : magnifierLargeStep);
        const int newPos = boundsDown(qRound(mSelection.top() * devicePixelRatio + step), false);
        if (modifiers & Qt::AltModifier) {
            mSelection.setBottom(devicePixelRatioI * newPos + mSelection.height());
        } else {
            mSelection.moveTop(devicePixelRatioI * newPos);
        }
        update();
        break;
    }
    case Qt::Key_Left: {
        if(mDisableArrowKeys) {
            update();
            break;
        }
        const qreal step = (shiftPressed ? 1 : magnifierLargeStep);
        const int newPos = boundsLeft(qRound(mSelection.left() * devicePixelRatio - step), false);
        if (modifiers & Qt::AltModifier) {
            mSelection.setRight(devicePixelRatioI * newPos + mSelection.width());
            mSelection = mSelection.normalized();
        } else {
            mSelection.moveLeft(devicePixelRatioI * newPos);
        }
        update();
        break;
    }
    default:
        break;
    }
    event->accept();
}

void QuickEditor::keyReleaseEvent(QKeyEvent* event)
{
    if (mToggleMagnifier && !(event->modifiers() & Qt::ShiftModifier)) {
        mToggleMagnifier = false;
        update();
    }
    event->accept();
}

int QuickEditor::boundsLeft(int newTopLeftX, const bool mouse)
{
    if (newTopLeftX < 0) {
        if (mouse) {
            // tweak startPos to prevent rectangle from getting stuck
            mStartPos.setX(mStartPos.x() + newTopLeftX * devicePixelRatioI);
        }
        newTopLeftX = 0;
    }

    return newTopLeftX;
}

int QuickEditor::boundsRight(int newTopLeftX, const bool mouse)
{
    // the max X coordinate of the top left point
    const int realMaxX = qRound((width() - mSelection.width()) * devicePixelRatioF());
    const int xOffset = newTopLeftX - realMaxX;
    if (xOffset > 0) {
        if (mouse) {
            mStartPos.setX(mStartPos.x() + xOffset * devicePixelRatioI);
        }
        newTopLeftX = realMaxX;
    }

    return newTopLeftX;
}

int QuickEditor::boundsUp(int newTopLeftY, const bool mouse)
{
    if (newTopLeftY < 0) {
        if (mouse) {
            mStartPos.setY(mStartPos.y() + newTopLeftY * devicePixelRatioI);
        }
        newTopLeftY = 0;
    }

    return newTopLeftY;
}

int QuickEditor::boundsDown(int newTopLeftY, const bool mouse)
{
    // the max Y coordinate of the top left point
    const int realMaxY = qRound((height() - mSelection.height()) * devicePixelRatio);
    const int yOffset = newTopLeftY - realMaxY;
    if (yOffset > 0) {
        if (mouse) {
            mStartPos.setY(mStartPos.y() + yOffset * devicePixelRatioI);
        }
        newTopLeftY = realMaxY;
    }

    return newTopLeftY;
}

void QuickEditor::mousePressEvent(QMouseEvent* event)
{
    if(event->source() == Qt::MouseEventNotSynthesized) {
        mHandleRadius = handleRadiusMouse;
    } else {
        mHandleRadius = handleRadiusTouch;
    }

    if (event->button() & Qt::LeftButton) {
        /* NOTE  Workaround for Bug 407843
        * If we show the selection Widget when a right click menu is open we lose focus on X.
        * When the user clicks we get the mouse back. We can only grab the keyboard if we already
        * have mouse focus. So just grab it undconditionally here.
        */
        grabKeyboard();
        const QPointF& pos = event->pos();
        mMousePos = pos;
        mMagnifierAllowed = true;
        mMouseDragState = mouseLocation(pos);
        mDisableArrowKeys = true;
        switch(mMouseDragState) {
        case MouseState::Outside:
            mStartPos = mMousePos;
            break;
        case MouseState::Inside:
            mStartPos = mMousePos;
            mMagnifierAllowed = false;
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
    if (mMagnifierAllowed) {
        update();
    }
    event->accept();
}

void QuickEditor::mouseMoveEvent(QMouseEvent* event)
{
    const QPointF& pos = event->pos();
    mMousePos = pos;
    mMagnifierAllowed = true;
    switch (mMouseDragState) {
    case MouseState::None: {
        setMouseCursor(pos);
        mMagnifierAllowed = false;
        break;
    }
    case MouseState::TopLeft:
    case MouseState::TopRight:
    case MouseState::BottomRight:
    case MouseState::BottomLeft: {
        const bool afterX = pos.x() >= mStartPos.x();
        const bool afterY = pos.y() >= mStartPos.y();
        mSelection.setRect(
            afterX ? mStartPos.x() : pos.x(),
            afterY ? mStartPos.y() : pos.y(),
            qAbs(pos.x() - mStartPos.x()) + (afterX ? devicePixelRatioI : 0),
            qAbs(pos.y() - mStartPos.y()) + (afterY ? devicePixelRatioI : 0)
        );
        update();
        break;
    }
    case MouseState::Outside: {
        mSelection.setRect(
            qMin(pos.x(), mStartPos.x()),
            qMin(pos.y(), mStartPos.y()),
            qAbs(pos.x() - mStartPos.x()) + devicePixelRatioI,
            qAbs(pos.y() - mStartPos.y()) + devicePixelRatioI
        );
        update();
        break;
    }
    case MouseState::Top:
    case MouseState::Bottom: {
        const bool afterY = pos.y() >= mStartPos.y();
        mSelection.setRect(
            mSelection.x(),
            afterY ? mStartPos.y() : pos.y(),
            mSelection.width(),
            qAbs(pos.y() - mStartPos.y()) + (afterY ? devicePixelRatioI : 0)
        );
        update();
        break;
    }
    case MouseState::Right:
    case MouseState::Left: {
        const bool afterX = pos.x() >= mStartPos.x();
        mSelection.setRect(
            afterX ? mStartPos.x() : pos.x(),
            mSelection.y(),
            qAbs(pos.x() - mStartPos.x()) + (afterX ? devicePixelRatioI : 0),
            mSelection.height()
        );
        update();
        break;
    }
    case MouseState::Inside: {
        mMagnifierAllowed = false;
        // We use some math here to figure out if the diff with which we
        // move the rectangle with moves it out of bounds,
        // in which case we adjust the diff to not let that happen

        // new top left point of the rectangle
        QPoint newTopLeft = ((pos - mStartPos + mInitialTopLeft) * devicePixelRatio).toPoint();

        auto newRect = QRect(newTopLeft, mSelection.size() * devicePixelRatio);

        auto screenBoundingRect = mScreenRegion.boundingRect();
        screenBoundingRect = QRect(screenBoundingRect.topLeft(), screenBoundingRect.size());
        if (!screenBoundingRect.contains(newRect)) {
            // Keep the item inside the scene screen region bounding rect.
            newTopLeft.setX(qMin(screenBoundingRect.right() - newRect.width(), qMax(newTopLeft.x(), screenBoundingRect.left())));
            newTopLeft.setY(qMin(screenBoundingRect.bottom() - newRect.height(), qMax(newTopLeft.y(), screenBoundingRect.top())));
        }

        const auto newTopLeftF = newTopLeft * devicePixelRatioI;
        mSelection.moveTo(newTopLeftF);
        update();
        break;
    }
    default:
        break;
    }

    event->accept();
}

void QuickEditor::mouseReleaseEvent(QMouseEvent* event)
{
    const auto button = event->button();
    if (button == Qt::LeftButton) {
        mDisableArrowKeys = false;
        if (mMouseDragState == MouseState::Inside) {
            setCursor(Qt::OpenHandCursor);
        } else if (mMouseDragState == MouseState::Outside && mReleaseToCapture) {
            event->accept();
            mMouseDragState = MouseState::None;
            return acceptSelection();
        }
    } else if (button == Qt::RightButton) {
        mSelection.setWidth(0);
        mSelection.setHeight(0);
    }
    event->accept();
    mMouseDragState = MouseState::None;
    update();
}

void QuickEditor::mouseDoubleClickEvent(QMouseEvent* event)
{
    event->accept();
    if (event->button() == Qt::LeftButton && mSelection.contains(event->pos())) {
        acceptSelection();
    }
}

QMap<ComparableQPoint, ComparableQPoint> QuickEditor::computeCoordinatesAfterScaling(QMap<ComparableQPoint, QPair<qreal, QSize>> outputsRect)
{
    QMap<ComparableQPoint, ComparableQPoint> translationMap;

    for (auto i = outputsRect.keyBegin(); i != outputsRect.keyEnd(); ++i) {
        translationMap.insert(*i, *i);
    }

    for (auto i = outputsRect.constBegin(); i != outputsRect.constEnd(); ++i) {
        const auto p = i.key();
        const auto size = i.value().second;
        const auto dpr = i.value().first;
        if (!qFuzzyCompare(dpr, 1.0)) {
            // must update all coordinates of next rects
            int newWidth = size.width();
            int newHeight = size.height();

            int deltaX = newWidth - (size.width());
            int deltaY = newHeight - (size.height());

            // for the next size
            for (auto i2 = outputsRect.constFind(p); i2 != outputsRect.constEnd(); ++i2) {

                auto point = i2.key();
                auto finalPoint = translationMap.value(point);

                if (point.x() >= newWidth + p.x() - deltaX) {
                    finalPoint.setX(finalPoint.x() + deltaX);
                }
                if (point.y() >= newHeight + p.y() - deltaY) {
                    finalPoint.setY(finalPoint.y() + deltaY);
                }
                // update final position point with the necessary deltas
                translationMap.insert(point, finalPoint);
            }
        }
    }

    return translationMap;
}


void QuickEditor::preparePaint()
{
    auto screens = QGuiApplication::screens();
    for (auto i = mImages.constBegin(); i != mImages.constEnd(); ++i) {
        QImage screenImage = i.value();
        const auto &pos = i.key();

        auto item = std::find_if(screens.constBegin(), screens.constEnd(),
                                      [pos] (const QScreen* screen){
            return screen->geometry().topLeft() == pos;
        });
        const QScreen* screen = *item;

        const qreal dpr = screenImage.width() / static_cast<qreal>(screen->geometry().width());
        mRectToDpr.append( QPair<QRect, qreal>(screen->geometry(), dpr));

        QRect virtualScreenRect;
        if (KWindowSystem::isPlatformX11()) {
            virtualScreenRect = QRect(pos, screenImage.size());
        } else {
            virtualScreenRect = QRect(pos, screenImage.size() / dpr);
        }
        mScreenRegion = mScreenRegion.united(virtualScreenRect);
    }
}

void QuickEditor::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    painter.eraseRect(rect());

    auto screens = QGuiApplication::screens();
    for (auto i = mImages.constBegin(); i != mImages.constEnd(); ++i) {
        QImage screenImage = i.value();
        const auto &pos = i.key();

        auto item = std::find_if(screens.constBegin(), screens.constEnd(),
                                      [pos] (const QScreen* screen){
            return screen->geometry().topLeft() == pos;
        });
        const QScreen* screen = *item;

        const qreal dpr = screenImage.width() / static_cast<qreal>(screen->geometry().width());
        const qreal dprI = 1.0 / dpr;

        QBrush brush(screenImage);

        brush.setTransform(QTransform().scale(dprI, dprI));

        painter.setBrushOrigin(screen->geometry().topLeft() / devicePixelRatio);

        QRect rectToDraw = screen->geometry();
        rectToDraw.moveTopLeft(rectToDraw.topLeft() / devicePixelRatio);
        rectToDraw.setSize(rectToDraw.size() * devicePixelRatio);
        painter.fillRect(rectToDraw, brush);
    }

    if (!mSelection.size().isEmpty() || mMouseDragState != MouseState::None) {
        const QRectF innerRect = mSelection.adjusted(1, 1, -1, -1);
        if (innerRect.width() > 0 && innerRect.height() > 0) {
            painter.setPen(mStrokeColor);
            painter.drawLine(mSelection.topLeft(), mSelection.topRight());
            painter.drawLine(mSelection.bottomRight(), mSelection.topRight());
            painter.drawLine(mSelection.bottomRight(), mSelection.bottomLeft());
            painter.drawLine(mSelection.bottomLeft(), mSelection.topLeft());
        }

        QRectF top(0, 0, width(), mSelection.top());
        QRectF right(mSelection.right(), mSelection.top(), width() - mSelection.right(), mSelection.height());
        QRectF bottom(0, mSelection.bottom() + 1, width(), height() - mSelection.bottom());
        QRectF left(0, mSelection.top(), mSelection.left(), mSelection.height());
        for (const auto& rect : { top, right, bottom, left }) {
            painter.fillRect(rect, mMaskColor);
        }

        bool dragHandlesVisible = false;
        if (mMouseDragState == MouseState::None) {
            dragHandlesVisible = true;
            drawDragHandles(painter);
        } else if (mMagnifierAllowed && (mShowMagnifier ^ mToggleMagnifier)) {
            drawMagnifier(painter);
        }
        drawSelectionSizeTooltip(painter, dragHandlesVisible);
        drawBottomHelpText(painter);
    } else {
        drawMidHelpText(painter);
    }
}

void QuickEditor::layoutBottomHelpText()
{
    int maxRightWidth = 0;
    int contentWidth = 0;
    int contentHeight = 0;
    mBottomHelpGridLeftWidth = 0;
    for (int i = 0; i < mbottomHelpLength; i++) {
        const auto& item = mBottomHelpText[i];
        const auto& left = item.first;
        const auto& right = item.second;
        const auto leftSize = left.size().toSize();
        mBottomHelpGridLeftWidth = qMax(mBottomHelpGridLeftWidth, leftSize.width());
        for (const auto& item : right) {
            const auto rightItemSize = item.size().toSize();
            maxRightWidth = qMax(maxRightWidth, rightItemSize.width());
            contentHeight += rightItemSize.height();
        }
        contentWidth = qMax(contentWidth, mBottomHelpGridLeftWidth + maxRightWidth + bottomHelpBoxPairSpacing);
        contentHeight += (i != bottomHelpMaxLength ? bottomHelpBoxMarginBottom : 0);
    }
    mBottomHelpContentPos.setX((mPrimaryScreenGeo.width() - contentWidth) / 2 + mPrimaryScreenGeo.x() / devicePixelRatio);
    mBottomHelpContentPos.setY((mPrimaryScreenGeo.height() + mPrimaryScreenGeo.y() / devicePixelRatio) - contentHeight - 8 );
    mBottomHelpGridLeftWidth += mBottomHelpContentPos.x();
    mBottomHelpBorderBox.setRect(
        mBottomHelpContentPos.x() - bottomHelpBoxPaddingX,
        mBottomHelpContentPos.y() - bottomHelpBoxPaddingY,
        contentWidth + bottomHelpBoxPaddingX * 2,
        contentHeight + bottomHelpBoxPaddingY * 2 - 1
    );
}

void QuickEditor::setBottomHelpText() {
    if (mReleaseToCapture && mSelection.size().isEmpty()) {
        //Release to capture enabled and NO saved region available
        mbottomHelpLength = 3;
        mBottomHelpText[0] = { QStaticText(i18n("Take Screenshot:")), { QStaticText(i18nc("Mouse action", "Release left-click")), QStaticText(i18nc("Keyboard action", "Enter")) } };
        mBottomHelpText[1] = { QStaticText(i18n("Create new selection rectangle:")), { QStaticText(i18nc("Mouse action", "Drag outside selection rectangle")), QStaticText(i18nc("Keyboard action", "+ Shift: Magnifier"))} };
        mBottomHelpText[2] = { QStaticText(i18n("Cancel:")), { QStaticText(i18nc("Keyboard action", "Escape")) } };
    } else {
        //Default text, Release to capture option disabled
        mBottomHelpText[0] = { QStaticText(i18n("Take Screenshot:")), { QStaticText(i18nc("Mouse action", "Double-click")), QStaticText(i18nc("Keyboard action", "Enter")) } };
        mBottomHelpText[1] = { QStaticText(i18n("Create new selection rectangle:")), { QStaticText(i18nc("Mouse action", "Drag outside selection rectangle")), QStaticText(i18nc("Keyboard action", "+ Shift: Magnifier"))} };
        mBottomHelpText[2] = { QStaticText(i18n("Move selection rectangle:")), { QStaticText(i18nc("Mouse action", "Drag inside selection rectangle")), QStaticText(i18nc("Keyboard action", "Arrow keys")), QStaticText(i18nc("Keyboard action", "+ Shift: Move in 1 pixel steps"))} };
        mBottomHelpText[3] = { QStaticText(i18n("Resize selection rectangle:")), { QStaticText(i18nc("Mouse action", "Drag handles")), QStaticText(i18nc("Keyboard action", "Arrow keys + Alt")), QStaticText(i18nc("Keyboard action", "+ Shift: Resize in 1 pixel steps"))} };
        mBottomHelpText[4] = { QStaticText(i18n("Reset selection:")), { QStaticText(i18nc("Mouse action", "Right-click")) } };
        mBottomHelpText[5] = { QStaticText(i18n("Cancel:")), { QStaticText(i18nc("Keyboard action", "Escape")) } };
    }
}

void QuickEditor::drawBottomHelpText(QPainter &painter)
{
    if (mSelection.intersects(mBottomHelpBorderBox)) {
        return;
    }

    painter.setBrush(mLabelBackgroundColor);
    painter.setPen(mLabelForegroundColor);
    painter.setFont(mBottomHelpTextFont);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.drawRect(mBottomHelpBorderBox);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int topOffset = mBottomHelpContentPos.y();
    for (int i = 0; i < mbottomHelpLength; i++) {
        const auto& item = mBottomHelpText[i];
        const auto& left = item.first;
        const auto& right = item.second;
        const auto leftSize = left.size().toSize();
        painter.drawStaticText(mBottomHelpGridLeftWidth - leftSize.width(), topOffset, left);
        for (const auto& item : right) {
            const auto rightItemSize = item.size().toSize();
            painter.drawStaticText(mBottomHelpGridLeftWidth + bottomHelpBoxPairSpacing, topOffset, item);
            topOffset += rightItemSize.height();
        }
        if (i != bottomHelpMaxLength) {
            topOffset += bottomHelpBoxMarginBottom;
        }
    }
}

void QuickEditor::drawDragHandles(QPainter &painter)
{
    // Rectangular region
    const qreal left = mSelection.x();
    const qreal centerX = left + mSelection.width() / 2.0;
    const qreal right = left + mSelection.width();
    const qreal top = mSelection.y();
    const qreal centerY = top + mSelection.height() / 2.0;
    const qreal bottom = top + mSelection.height();

    // rectangle too small: make handles free-floating
    qreal offset = 0;
    // rectangle too close to screen edges: move handles on that edge inside the rectangle, so they're still visible
    qreal offsetTop = 0;
    qreal offsetRight = 0;
    qreal offsetBottom = 0;
    qreal offsetLeft = 0;

    const qreal minDragHandleSpace = 4 * mHandleRadius + 2 * minSpacingBetweenHandles;
    const qreal minEdgeLength = qMin(mSelection.width(), mSelection.height());
    if (minEdgeLength < minDragHandleSpace) {
        offset = (minDragHandleSpace - minEdgeLength) / 2.0;
    } else {
        QRect virtualScreenGeo = QGuiApplication::primaryScreen()->virtualGeometry();
        const int penWidth = painter.pen().width();

        offsetTop = top - virtualScreenGeo.top() - mHandleRadius;
        offsetTop = (offsetTop >= 0) ? 0 : offsetTop;

        offsetRight =  virtualScreenGeo.right() - right - mHandleRadius + penWidth;
        offsetRight = (offsetRight >= 0) ? 0 : offsetRight;

        offsetBottom = virtualScreenGeo.bottom() - bottom - mHandleRadius + penWidth;
        offsetBottom = (offsetBottom >= 0) ? 0 : offsetBottom;

        offsetLeft = left - virtualScreenGeo.left() - mHandleRadius;
        offsetLeft = (offsetLeft >= 0) ? 0 : offsetLeft;
    }

    //top-left handle
    this->mHandlePositions[0] = QPointF {left - offset - offsetLeft,  top - offset - offsetTop};
    //top-right handle
    this->mHandlePositions[1] = QPointF {right + offset + offsetRight, top - offset - offsetTop};
    // bottom-right handle
    this->mHandlePositions[2] = QPointF {right + offset + offsetRight, bottom + offset + offsetBottom};
    // bottom-left
    this->mHandlePositions[3] = QPointF {left - offset - offsetLeft, bottom + offset + offsetBottom};
    // top-center handle
    this->mHandlePositions[4] = QPointF {centerX, top - offset - offsetTop};
    // right-center handle
    this->mHandlePositions[5] = QPointF {right + offset + offsetRight, centerY};
    // bottom-center handle
    this->mHandlePositions[6] = QPointF {centerX, bottom + offset + offsetBottom};
    // left-center handle
    this->mHandlePositions[7] = QPointF {left - offset - offsetLeft, centerY};

    // start path
    QPainterPath path;

    // add handles to the path
    for (const QPointF &handlePosition : this->mHandlePositions) {
        path.addEllipse(handlePosition, mHandleRadius, mHandleRadius);
    }

    // draw the path
    painter.fillPath(path, mStrokeColor);
}

void QuickEditor::drawMagnifier(QPainter &painter)
{
    const int pixels = 2 * magPixels + 1;
    int magX = static_cast<int>(mMousePos.x() * devicePixelRatio - magPixels);
    int offsetX = 0;
    if (magX < 0) {
        offsetX = magX;
        magX = 0;
    } else {
        const int maxX = mPixmap.width() - pixels;
        if (magX > maxX) {
            offsetX = magX - maxX;
            magX = maxX;
        }
    }
    int magY = static_cast<int>(mMousePos.y() * devicePixelRatio - magPixels);
    int offsetY = 0;
    if (magY < 0) {
        offsetY = magY;
        magY = 0;
    } else {
        const int maxY = mPixmap.height() - pixels;
        if (magY > maxY) {
            offsetY = magY - maxY;
            magY = maxY;
        }
    }
    QRectF magniRect(magX, magY, pixels, pixels);

    qreal drawPosX = mMousePos.x() + magOffset + pixels * magZoom / 2;
    if (drawPosX > width() - pixels * magZoom / 2) {
        drawPosX = mMousePos.x() - magOffset - pixels * magZoom / 2;
    }
    qreal drawPosY = mMousePos.y() + magOffset + pixels * magZoom / 2;
    if (drawPosY > height() - pixels * magZoom / 2) {
        drawPosY = mMousePos.y() - magOffset - pixels * magZoom / 2;
    }
    QPointF drawPos(drawPosX, drawPosY);
    QRectF crossHairTop(drawPos.x() + magZoom * (offsetX - 0.5), drawPos.y() - magZoom * (magPixels + 0.5), magZoom, magZoom * (magPixels + offsetY));
    QRectF crossHairRight(drawPos.x() + magZoom * (0.5 + offsetX), drawPos.y() + magZoom * (offsetY - 0.5), magZoom * (magPixels - offsetX), magZoom);
    QRectF crossHairBottom(drawPos.x() + magZoom * (offsetX - 0.5), drawPos.y() + magZoom * (0.5 + offsetY), magZoom, magZoom * (magPixels - offsetY));
    QRectF crossHairLeft(drawPos.x() - magZoom * (magPixels + 0.5), drawPos.y() + magZoom * (offsetY - 0.5), magZoom * (magPixels + offsetX), magZoom);
    QRectF crossHairBorder(drawPos.x() - magZoom * (magPixels + 0.5) - 1, drawPos.y() - magZoom * (magPixels + 0.5) - 1, pixels * magZoom + 2, pixels * magZoom + 2);
    const auto frag = QPainter::PixmapFragment::create(drawPos, magniRect, magZoom, magZoom);

    painter.fillRect(crossHairBorder, mLabelForegroundColor);
    painter.drawPixmapFragments(&frag, 1, mPixmap, QPainter::OpaqueHint);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    for (auto& rect : { crossHairTop, crossHairRight, crossHairBottom, crossHairLeft }) {
        painter.fillRect(rect, mCrossColor);
    }
}

void QuickEditor::drawMidHelpText(QPainter &painter)
{
    painter.fillRect(rect(), mMaskColor);
    painter.setFont(mMidHelpTextFont);
    QRect textSize = painter.boundingRect(QRect(), Qt::AlignCenter, mMidHelpText);
    QPoint pos((mPrimaryScreenGeo.width() - textSize.width()) / 2 + mPrimaryScreenGeo.x() / devicePixelRatio,
               (mPrimaryScreenGeo.height() - textSize.height()) / 2 + mPrimaryScreenGeo.y() / devicePixelRatio);

    painter.setBrush(mLabelBackgroundColor);
    QPen pen(mLabelForegroundColor);
    pen.setWidth(2);
    painter.setPen(pen);
    painter.drawRoundedRect(QRect(pos.x() - 20, pos.y() - 20, textSize.width() + 40, textSize.height() + 40), 4, 4);

    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.drawText(QRect(pos, textSize.size()), Qt::AlignCenter, mMidHelpText);
}

void QuickEditor::drawSelectionSizeTooltip(QPainter &painter, bool dragHandlesVisible)
{
    // Set the selection size and finds the most appropriate position:
    // - vertically centered inside the selection if the box is not covering the a large part of selection
    // - on top of the selection if the selection x position fits the box height plus some margin
    // - at the bottom otherwise
    QString selectionSizeText = ki18n("%1×%2").subs(qRound(mSelection.width() * devicePixelRatio)).subs(qRound(mSelection.height() * devicePixelRatio)).toString();
    const QRect selectionSizeTextRect = painter.boundingRect(QRect(), 0, selectionSizeText);

    const int selectionBoxWidth = selectionSizeTextRect.width() + selectionBoxPaddingX * 2;
    const int selectionBoxHeight = selectionSizeTextRect.height() + selectionBoxPaddingY * 2;
    const int selectionBoxX = qBound(
        0,
        static_cast<int>(mSelection.x()) + (static_cast<int>(mSelection.width()) - selectionSizeTextRect.width()) / 2 - selectionBoxPaddingX,
        width() - selectionBoxWidth
    );
    int selectionBoxY;
    if ((mSelection.width() >= selectionSizeThreshold) && (mSelection.height() >= selectionSizeThreshold)) {
        // show inside the box
        selectionBoxY = static_cast<int>(mSelection.y() + (mSelection.height() - selectionSizeTextRect.height()) / 2);
    } else {
        // show on top by default, above the drag Handles if they're visible
        if (dragHandlesVisible) {
            selectionBoxY = static_cast<int>(mHandlePositions[4].y() - mHandleRadius - selectionBoxHeight - selectionBoxMarginY);
            if (selectionBoxY < 0) {
                selectionBoxY = static_cast<int>(mHandlePositions[6].y() + mHandleRadius + selectionBoxMarginY);
            }
        } else {
            selectionBoxY = static_cast<int>(mSelection.y() - selectionBoxHeight - selectionBoxMarginY);
            if (selectionBoxY < 0) {
                selectionBoxY = static_cast<int>(mSelection.y() + mSelection.height() + selectionBoxMarginY);
            }
        }
    }

    // Now do the actual box, border, and text drawing
    painter.setBrush(mLabelBackgroundColor);
    painter.setPen(mLabelForegroundColor);
    const QRect selectionBoxRect(
        selectionBoxX,
        selectionBoxY,
        selectionBoxWidth,
        selectionBoxHeight
    );

    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.drawRect(selectionBoxRect);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.drawText(selectionBoxRect, Qt::AlignCenter, selectionSizeText);
}

void QuickEditor::setMouseCursor(const QPointF& pos)
{
    MouseState mouseState = mouseLocation(pos);
    if (mouseState == MouseState::Outside) {
        setCursor(Qt::CrossCursor);
    } else if (MouseState::TopLeftOrBottomRight & mouseState) {
        setCursor(Qt::SizeFDiagCursor);
    } else if (MouseState::TopRightOrBottomLeft & mouseState) {
        setCursor(Qt::SizeBDiagCursor);
    } else if (MouseState::TopOrBottom & mouseState) {
        setCursor(Qt::SizeVerCursor);
    } else if (MouseState::RightOrLeft & mouseState) {
        setCursor(Qt::SizeHorCursor);
    } else {
        setCursor(Qt::OpenHandCursor);
    }
}

QuickEditor::MouseState QuickEditor::mouseLocation(const QPointF& pos)
{
    auto isPointInsideCircle = [](const QPointF & circleCenter, qreal radius, const QPointF & point) {
        return (qPow(point.x() - circleCenter.x(), 2) + qPow(point.y() - circleCenter.y(), 2) <= qPow(radius, 2)) ? true : false;
    };

    if (isPointInsideCircle(mHandlePositions[0], mHandleRadius * increaseDragAreaFactor, pos)) {
        return MouseState::TopLeft;
    } else if (isPointInsideCircle(mHandlePositions[1], mHandleRadius * increaseDragAreaFactor, pos)) {
        return MouseState::TopRight;
    } else if (isPointInsideCircle(mHandlePositions[2], mHandleRadius * increaseDragAreaFactor, pos)) {
        return MouseState::BottomRight;
    } else if (isPointInsideCircle(mHandlePositions[3], mHandleRadius * increaseDragAreaFactor, pos)) {
        return MouseState::BottomLeft;
    } else if (isPointInsideCircle(mHandlePositions[4], mHandleRadius * increaseDragAreaFactor, pos)) {
        return MouseState::Top;
    } else if (isPointInsideCircle(mHandlePositions[5], mHandleRadius * increaseDragAreaFactor, pos)) {
        return MouseState::Right;
    } else if (isPointInsideCircle(mHandlePositions[6], mHandleRadius * increaseDragAreaFactor, pos)) {
        return MouseState::Bottom;
    } else if (isPointInsideCircle(mHandlePositions[7], mHandleRadius * increaseDragAreaFactor, pos)) {
        return MouseState::Left;
    }

    auto inRange = [](qreal low, qreal high, qreal value) {
      return value >= low && value <= high;
    };

    auto withinThreshold = [](qreal offset, qreal threshold) {
      return qFabs(offset) <= threshold;
    };

    //Rectangle can be resized when border is dragged, if it's big enough
    if (mSelection.width() >= 100 && mSelection.height() >= 100) {
        if (inRange(mSelection.x(), mSelection.x() + mSelection.width(), pos.x())) {
            if (withinThreshold(pos.y() - mSelection.y(), borderDragAreaSize)) {
                return MouseState::Top;
            } else if (withinThreshold(pos.y() - mSelection.y() - mSelection.height(), borderDragAreaSize)) {
                return MouseState::Bottom;
            }
        }
        if (inRange(mSelection.y(), mSelection.y() + mSelection.height(), pos.y())) {
            if (withinThreshold(pos.x() - mSelection.x(), borderDragAreaSize)) {
                return MouseState::Left;
            } else if (withinThreshold(pos.x() - mSelection.x() - mSelection.width(), borderDragAreaSize)) {
                return MouseState::Right;
            }
        }
    }
    if (mSelection.contains(pos.toPoint())) {
        return MouseState::Inside;
    } else {
        return MouseState::Outside;
    }
}
