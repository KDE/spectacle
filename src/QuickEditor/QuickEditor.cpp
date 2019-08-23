/*
 *  Copyright (C) 2018 Ambareesh "Amby" Balaji <ambareeshbalaji@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include <KLocalizedString>
#include <QGuiApplication>
#include <QScreen>
#include <QtCore/qmath.h>

#include "QuickEditor.h"
#include "SpectacleConfig.h"

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

QuickEditor::QuickEditor(const QPixmap& thePixmap, QWidget *parent) :
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
    mPixmap(thePixmap),
    mMagnifierAllowed(false),
    mShowMagnifier(SpectacleConfig::instance()->showMagnifierChecked()),
    mToggleMagnifier(false),
    mReleaseToCapture(SpectacleConfig::instance()->useReleaseToCapture()),
    mRememberRegion(SpectacleConfig::instance()->alwaysRememberRegion() || SpectacleConfig::instance()->rememberLastRectangularRegion()),
    mDisableArrowKeys(false),
    mPrimaryScreenGeo(QGuiApplication::primaryScreen()->geometry()),
    mbottomHelpLength(bottomHelpMaxLength),
    mHandleRadius(handleRadiusMouse)
{
    SpectacleConfig *config = SpectacleConfig::instance();
    if (config->useLightRegionMaskColour()) {
        mMaskColor = QColor(255, 255, 255, 100);
    }

    setMouseTracking(true);
    setAttribute(Qt::WA_StaticContents);
    setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::Popup | Qt::WindowStaysOnTopHint);

    dprI = 1.0 / devicePixelRatioF();
    setGeometry(0, 0, static_cast<int>(mPixmap.width() * dprI), static_cast<int>(mPixmap.height() * dprI));

    if (config->rememberLastRectangularRegion()) {
        QRect cropRegion = config->cropRegion();
        if (!cropRegion.isEmpty()) {
            mSelection = QRectF(
                cropRegion.x() * dprI,
                cropRegion.y() * dprI,
                cropRegion.width() * dprI,
                cropRegion.height() * dprI
            ).intersected(geometry());
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

    update();
}

void QuickEditor::acceptSelection()
{
    if (!mSelection.isEmpty()) {
        const qreal dpr = devicePixelRatioF();
        QRect scaledCropRegion = QRect(
            qRound(mSelection.x() * dpr),
            qRound(mSelection.y() * dpr),
            qRound(mSelection.width() * dpr),
            qRound(mSelection.height() * dpr)
        );
        SpectacleConfig::instance()->setCropRegion(scaledCropRegion);
        emit grabDone(mPixmap.copy(scaledCropRegion));
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
        const int newPos = boundsUp(qRound(mSelection.top() * devicePixelRatioF() - step), false);
        if (modifiers & Qt::AltModifier) {
            mSelection.setBottom(dprI * newPos + mSelection.height());
            mSelection = mSelection.normalized();
        } else {
            mSelection.moveTop(dprI * newPos);
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
        const int newPos = boundsRight(qRound(mSelection.left() * devicePixelRatioF() + step), false);
        if (modifiers & Qt::AltModifier) {
            mSelection.setRight(dprI * newPos + mSelection.width());
        } else {
            mSelection.moveLeft(dprI * newPos);
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
        const int newPos = boundsDown(qRound(mSelection.top() * devicePixelRatioF() + step), false);
        if (modifiers & Qt::AltModifier) {
            mSelection.setBottom(dprI * newPos + mSelection.height());
        } else {
            mSelection.moveTop(dprI * newPos);
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
        const int newPos = boundsLeft(qRound(mSelection.left() * devicePixelRatioF() - step), false);
        if (modifiers & Qt::AltModifier) {
            mSelection.setRight(dprI * newPos + mSelection.width());
            mSelection = mSelection.normalized();
        } else {
            mSelection.moveLeft(dprI * newPos);
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
            mStartPos.setX(mStartPos.x() + newTopLeftX * dprI);
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
            mStartPos.setX(mStartPos.x() + xOffset * dprI);
        }
        newTopLeftX = realMaxX;
    }

    return newTopLeftX;



}

int QuickEditor::boundsUp(int newTopLeftY, const bool mouse)
{
    if (newTopLeftY < 0) {
        if (mouse) {
            mStartPos.setY(mStartPos.y() + newTopLeftY * dprI);
        }
        newTopLeftY = 0;
    }

    return newTopLeftY;
}

int QuickEditor::boundsDown(int newTopLeftY, const bool mouse)
{
    // the max Y coordinate of the top left point
    const int realMaxY = qRound((height() - mSelection.height()) * devicePixelRatioF());
    const int yOffset = newTopLeftY - realMaxY;
    if (yOffset > 0) {
        if (mouse) {
            mStartPos.setY(mStartPos.y() + yOffset * dprI);
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
            mStartPos = pos;
            break;
        case MouseState::Inside:
            mStartPos = pos;
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
            qAbs(pos.x() - mStartPos.x()) + (afterX ? dprI : 0),
            qAbs(pos.y() - mStartPos.y()) + (afterY ? dprI : 0)
        );
        update();
        break;
    }
    case MouseState::Outside: {
        mSelection.setRect(
            qMin(pos.x(), mStartPos.x()),
            qMin(pos.y(), mStartPos.y()),
            qAbs(pos.x() - mStartPos.x()) + dprI,
            qAbs(pos.y() - mStartPos.y()) + dprI
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
            qAbs(pos.y() - mStartPos.y()) + (afterY ? dprI : 0)
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
            qAbs(pos.x() - mStartPos.x()) + (afterX ? dprI : 0),
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

        const qreal dpr = devicePixelRatioF();
        // new top left point of the rectangle
        QPoint newTopLeft = ((pos - mStartPos + mInitialTopLeft) * dpr).toPoint();

        int newTopLeftX = boundsLeft(newTopLeft.x());
        if (newTopLeftX != 0) {
            newTopLeftX = boundsRight(newTopLeftX);
        }

        int newTopLeftY = boundsUp(newTopLeft.y());
        if (newTopLeftY != 0) {
            newTopLeftY = boundsDown(newTopLeftY);
        }

        const auto newTopLeftF = QPointF(newTopLeftX * dprI, newTopLeftY * dprI);

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
        if(mMouseDragState == MouseState::Inside) {
            setCursor(Qt::OpenHandCursor);
        }
        else if(mMouseDragState == MouseState::Outside && mReleaseToCapture) {
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

void QuickEditor::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing);
    QBrush brush(mPixmap);
    brush.setTransform(QTransform().scale(dprI, dprI));
    painter.setBackground(brush);
    painter.eraseRect(geometry());
    if (!mSelection.size().isEmpty() || mMouseDragState != MouseState::None) {
        painter.fillRect(mSelection, mStrokeColor);
        const QRectF innerRect = mSelection.adjusted(1, 1, -1, -1);
        if (innerRect.width() > 0 && innerRect.height() > 0) {
            painter.eraseRect(mSelection.adjusted(1, 1, -1, -1));
        }

        QRectF top(0, 0, width(), mSelection.top());
        QRectF right(mSelection.right(), mSelection.top(), width() - mSelection.right(), mSelection.height());
        QRectF bottom(0, mSelection.bottom(), width(), height() - mSelection.bottom());
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
    mBottomHelpContentPos.setX((mPrimaryScreenGeo.width() - contentWidth) / 2 + mPrimaryScreenGeo.x());
    mBottomHelpContentPos.setY(height() - contentHeight - 8);
    mBottomHelpGridLeftWidth += mBottomHelpContentPos.x();
    mBottomHelpBorderBox.setRect(
        mBottomHelpContentPos.x() - bottomHelpBoxPaddingX,
        mBottomHelpContentPos.y() - bottomHelpBoxPaddingY,
        contentWidth + bottomHelpBoxPaddingX * 2,
        contentHeight + bottomHelpBoxPaddingY * 2 - 1
    );
}

void QuickEditor::setBottomHelpText() {
    if (mReleaseToCapture) {
        if(mRememberRegion && !mSelection.size().isEmpty()) {
            //Release to capture enabled and saved region available
            mBottomHelpText[0] = {QStaticText(i18nc("Mouse action", "Click and drag,")),{QStaticText(i18n(" "))}};
            mBottomHelpText[1] = {QStaticText(i18nc("Keyboard/mouse action", "Enter, double-click:")),
                                  {QStaticText(i18n("Take screenshot"))}};
            mBottomHelpText[2] = {QStaticText(i18nc("Keyboard action", "Shift:")), {
                    QStaticText(i18nc("Shift key action first half", "Hold to toggle magnifier")),
                    QStaticText(i18nc("Shift key action second half", "while dragging selection handles"))
            }};
            mBottomHelpText[3] = {QStaticText(i18nc("Keyboard action", "Arrow keys:")), {
                    QStaticText(i18nc("Shift key action first line", "Move selection rectangle")),
                    QStaticText(i18nc("Shift key action second line", "Hold Alt to resize, Shift to fine‑tune"))
            }};
            mBottomHelpText[4] = {QStaticText(i18nc("Mouse action", "Right-click:")),
                                  {QStaticText(i18n("Reset selection"))}};
            mBottomHelpText[5] = {QStaticText(i18nc("Keyboard action", "Esc:")), {QStaticText(i18n("Cancel"))}};

        } else {
            //Release to capture enabled and NO saved region available
            mbottomHelpLength = 4;
            mBottomHelpText[0] = {QStaticText(i18nc("Keyboard/mouse action", "Release left-click, Enter:")),
                                  {QStaticText(i18n("Take Screenshot"))}};
            mBottomHelpText[1] = {QStaticText(i18nc("Keyboard action", "Shift:")), {
                    QStaticText(i18nc("Shift key action first half", "Hold to toggle magnifier"))}};
            mBottomHelpText[2] = {QStaticText(i18nc("Mouse action", "Right-click:")),
                                  {QStaticText(i18n("Reset selection"))}};
            mBottomHelpText[3] = {QStaticText(i18nc("Keyboard action", "Esc:")), {QStaticText(i18n("Cancel"))}};
        }
    }else {
        //Default text, Release to capture option disabled
        mbottomHelpLength = 5;
        mBottomHelpText[0] = {QStaticText(i18nc("Keyboard/mouse action", "Enter, double-click:")),
                              {QStaticText(i18n("Take screenshot"))}};
        mBottomHelpText[1] = {QStaticText(i18nc("Keyboard action", "Shift:")), {
                QStaticText(i18nc("Shift key action first half", "Hold to toggle magnifier")),
                QStaticText(i18nc("Shift key action second half", "while dragging selection handles"))
        }};
        mBottomHelpText[2] = {QStaticText(i18nc("Keyboard action", "Arrow keys:")), {
                QStaticText(i18nc("Shift key action first line", "Move selection rectangle")),
                QStaticText(i18nc("Shift key action second line", "Hold Alt to resize, Shift to fine‑tune"))
        }};
        mBottomHelpText[3] = {QStaticText(i18nc("Mouse action", "Right-click:")),
                              {QStaticText(i18n("Reset selection"))}};
        mBottomHelpText[4] = {QStaticText(i18nc("Keyboard action", "Esc:")), {QStaticText(i18n("Cancel"))}};
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
    int magX = static_cast<int>(mMousePos.x() * devicePixelRatioF() - magPixels);
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
    int magY = static_cast<int>(mMousePos.y() * devicePixelRatioF() - magPixels);
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
    painter.fillRect(geometry(), mMaskColor);
    painter.setFont(mMidHelpTextFont);
    QRect textSize = painter.boundingRect(QRect(), Qt::AlignCenter, mMidHelpText);
    QPoint pos((mPrimaryScreenGeo.width() - textSize.width()) / 2 + mPrimaryScreenGeo.x(), (height() - textSize.height()) / 2);

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
    const qreal dpr = devicePixelRatioF();
    QString selectionSizeText = ki18n("%1×%2").subs(qRound(mSelection.width() * dpr)).subs(qRound(mSelection.height() * dpr)).toString();
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
    if(mSelection.width() >= 100 && mSelection.height() >= 100) {
      if(inRange(mSelection.x(), mSelection.x() + mSelection.width(), pos.x())) {
        if(withinThreshold(pos.y() - mSelection.y(), borderDragAreaSize)) {
          return MouseState::Top;
        } else if(withinThreshold(pos.y() - mSelection.y() - mSelection.height(), borderDragAreaSize)) {
          return MouseState::Bottom;
        }
      }
      if(inRange(mSelection.y(), mSelection.y() + mSelection.height(), pos.y())) {
        if(withinThreshold(pos.x() - mSelection.x(), borderDragAreaSize)) {
          return MouseState::Left;
        } else if(withinThreshold(pos.x() - mSelection.x() - mSelection.width(), borderDragAreaSize)) {
          return MouseState::Right;
        }
      }
    }
    if (mSelection.contains(pos)) {
        return MouseState::Inside;
    } else {
        return MouseState::Outside;
    }
}
