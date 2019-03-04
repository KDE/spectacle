/*
 *  Copyright (C) 2016 Boudhayan Gupta <bgupta@kde.org>
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

#include "QuickEditor.h"
#include "SpectacleConfig.h"

const qreal QuickEditor::mouseAreaSize = 20.0;
const qreal QuickEditor::cornerHandleRadius = 8.0;
const qreal QuickEditor::midHandleRadius = 5.0;
const int QuickEditor::selectionSizeThreshold = 100;

const int QuickEditor::selectionBoxPaddingX = 5;
const int QuickEditor::selectionBoxPaddingY = 4;
const int QuickEditor::selectionBoxMarginY = 2;

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

QuickEditor::QuickEditor(const QPixmap& pixmap) :
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
    mBottomHelpText{
        {QStaticText(i18nc("Keyboard/mouse action", "Enter, double-click:")), {QStaticText(i18n("Take screenshot"))}},
        {QStaticText(i18nc("Keyboard action", "Shift:")), {
            QStaticText(i18nc("Shift key action first half", "Hold to toggle magnifier")),
            QStaticText(i18nc("Shift key action second half", "while dragging selection handles"))
        }},
        {QStaticText(i18nc("Keyboard action", "Arrow keys:")), {
            QStaticText(i18nc("Shift key action first line", "Move selection rectangle")),
            QStaticText(i18nc("Shift key action second line", "Hold Alt to resize, Shift to fine‑tune"))
        }},
        {QStaticText(i18nc("Mouse action", "Right-click:")), {QStaticText(i18n("Reset selection"))}},
        {QStaticText(i18nc("Keyboard action", "Esc:")), {QStaticText(i18n("Cancel"))}},
    },
    mBottomHelpTextFont(font()),
    mBottomHelpGridLeftWidth(0),
    mMouseDragState(MouseState::None),
    mPixmap(pixmap),
    mMagnifierAllowed(false),
    mShowMagnifier(SpectacleConfig::instance()->showMagnifierChecked()),
    mToggleMagnifier(false),
    mPrimaryScreenGeo(QGuiApplication::primaryScreen()->geometry())
{
    SpectacleConfig *config = SpectacleConfig::instance();
    if (config->useLightRegionMaskColour()) {
        mMaskColor = QColor(255, 255, 255, 100);
    }

    setMouseTracking(true);
    setAttribute(Qt::WA_StaticContents);
    setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::Popup | Qt::WindowStaysOnTopHint);
    show();

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

    mMidHelpTextFont.setPointSize(midHelpTextFontSize);
    if (!bottomHelpTextPrepared) {
        bottomHelpTextPrepared = true;
        const auto prepare = [this](QStaticText& item) {
            item.prepare(QTransform(), mBottomHelpTextFont);
            item.setPerformanceHint(QStaticText::AggressiveCaching);
        };
        for (auto& pair : mBottomHelpText) {
            prepare(pair.first);
            for (auto item : pair.second) {
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
    if (event->button() & Qt::LeftButton) {
        const QPointF& pos = event->pos();
        mMousePos = pos;
        mMagnifierAllowed = true;
        mMouseDragState = mouseLocation(pos);
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
    if (button == Qt::LeftButton && mMouseDragState == MouseState::Inside) {
        setCursor(Qt::OpenHandCursor);
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

        drawSelectionSizeTooltip(painter);
        if (mMouseDragState == MouseState::None) { // mouse is up
            if ((mSelection.width() > 20) && (mSelection.height() > 20)) {
                drawDragHandles(painter);
            }

        } else if (mMagnifierAllowed && (mShowMagnifier ^ mToggleMagnifier)) {
            drawMagnifier(painter);
        }
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
    int i = 0;
    for (const auto& item : mBottomHelpText) {
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
        contentHeight += (++i != bottomHelpLength ? bottomHelpBoxMarginBottom : 0);
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
    int i = 0;
    for (const auto& item : mBottomHelpText) {
        const auto& left = item.first;
        const auto& right = item.second;
        const auto leftSize = left.size().toSize();
        painter.drawStaticText(mBottomHelpGridLeftWidth - leftSize.width(), topOffset, left);
        for (const auto& item : right) {
            const auto rightItemSize = item.size().toSize();
            painter.drawStaticText(mBottomHelpGridLeftWidth + bottomHelpBoxPairSpacing, topOffset, item);
            topOffset += rightItemSize.height();
        }
        if (++i != bottomHelpLength) {
            topOffset += bottomHelpBoxMarginBottom;
        }
    }
}

void QuickEditor::drawDragHandles(QPainter& painter)
{
    const qreal left = mSelection.x();
    const qreal width = mSelection.width();
    const qreal centerX = left + width / 2.0;
    const qreal right = left + width;

    const qreal top = mSelection.y();
    const qreal height = mSelection.height();
    const qreal centerY = top + height / 2.0;
    const qreal bottom = top + height;

    // start a path
    QPainterPath path;

    const qreal cornerHandleDiameter = 2 * cornerHandleRadius;

    // x and y coordinates of handle arcs
    const qreal leftHandle = left - cornerHandleRadius;
    const qreal topHandle = top - cornerHandleRadius;
    const qreal rightHandle = right - cornerHandleRadius;
    const qreal bottomHandle = bottom - cornerHandleRadius;
    const qreal centerHandleX = centerX - midHandleRadius;
    const qreal centerHandleY = centerY - midHandleRadius;

    // top-left handle
    path.moveTo(left, top);
    path.arcTo(leftHandle, topHandle, cornerHandleDiameter, cornerHandleDiameter, 0, -90);

    // top-right handle
    path.moveTo(right, top);
    path.arcTo(rightHandle, topHandle, cornerHandleDiameter, cornerHandleDiameter, 180, 90);

    // bottom-left handle
    path.moveTo(left, bottom);
    path.arcTo(leftHandle, bottomHandle, cornerHandleDiameter, cornerHandleDiameter, 0, 90);

    // bottom-right handle
    path.moveTo(right, bottom);
    path.arcTo(rightHandle, bottomHandle, cornerHandleDiameter, cornerHandleDiameter, 180, -90);

    const qreal midHandleDiameter = 2 * midHandleRadius;
    // top-center handle
    path.moveTo(centerX, top);
    path.arcTo(centerHandleX, top - midHandleRadius, midHandleDiameter, midHandleDiameter, 0, -180);

    // right-center handle
    path.moveTo(right, centerY);
    path.arcTo(right - midHandleRadius, centerHandleY, midHandleDiameter, midHandleDiameter, 90, 180);

    // bottom-center handle
    path.moveTo(centerX, bottom);
    path.arcTo(centerHandleX, bottom - midHandleRadius, midHandleDiameter, midHandleDiameter, 0, 180);

    // left-center handle
    path.moveTo(left, centerY);
    path.arcTo(left - midHandleRadius, centerHandleY, midHandleDiameter, midHandleDiameter, 90, -180);

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

void QuickEditor::drawSelectionSizeTooltip(QPainter &painter)
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
        // show on top by default
        selectionBoxY = static_cast<int>(mSelection.y() - selectionBoxHeight - selectionBoxMarginY);
        if (selectionBoxY < 0) {
            // show at the bottom
            selectionBoxY = static_cast<int>(mSelection.y() + mSelection.height() + selectionBoxMarginY);
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
    if (mSelection.contains(pos)) {
        const qreal verSize = qMin(mouseAreaSize, mSelection.height() / 2);
        const qreal horSize = qMin(mouseAreaSize, mSelection.width() / 2);

        auto withinThreshold = [](const qreal offset, const qreal size) {
            return offset <= size && offset >= 0;
        };

        const bool withinTopEdge = withinThreshold(pos.y() - mSelection.top(), verSize);
        const bool withinRightEdge = withinThreshold(mSelection.right() - pos.x(), horSize);
        const bool withinBottomEdge = !withinTopEdge && withinThreshold(mSelection.bottom() - pos.y(), verSize);
        const bool withinLeftEdge = !withinRightEdge && withinThreshold(pos.x() - mSelection.left(), horSize);

        if (withinTopEdge) {
            if (withinRightEdge) {
                return MouseState::TopRight;
            } else if (withinLeftEdge) {
                return MouseState::TopLeft;
            } else {
                return MouseState::Top;
            }
        } else if (withinBottomEdge) {
            if (withinRightEdge) {
                return MouseState::BottomRight;
            } else if (withinLeftEdge) {
                return MouseState::BottomLeft;
            } else {
                return MouseState::Bottom;
            }
        } else if (withinRightEdge) {
            return MouseState::Right;
        } else if (withinLeftEdge) {
            return MouseState::Left;
        } else {
            return MouseState::Inside;
        }
    } else {
        return MouseState::Outside;
    }
}
