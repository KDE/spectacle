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

#ifndef QUICKEDITOR_H
#define QUICKEDITOR_H

#include <QKeyEvent>
#include <QPainter>
#include <QStaticText>
#include <QWidget>
#include <utility>
#include <vector>

#include "ComparableQPoint.h"

class QMouseEvent;

namespace KWayland {
namespace Client {
class PlasmaShell;
}
}

class QuickEditor: public QWidget
{
    Q_OBJECT

    public:

    explicit QuickEditor(const QMap<ComparableQPoint, QImage> &images, KWayland::Client::PlasmaShell *plasmashell, QWidget *parent = nullptr);
    virtual ~QuickEditor() = default;

    private:

    enum MouseState : short {
        None = 0, // 0000
        Inside = 1 << 0, // 0001
        Outside = 1 << 1, // 0010
        TopLeft = 5, //101
        Top = 17, // 10001
        TopRight = 9, // 1001
        Right = 33, // 100001
        BottomRight = 6, // 110
        Bottom = 18, // 10010
        BottomLeft = 10, // 1010
        Left = 34, // 100010
        TopLeftOrBottomRight = TopLeft & BottomRight, // 100
        TopRightOrBottomLeft = TopRight & BottomLeft, // 1000
        TopOrBottom = Top & Bottom, // 10000
        RightOrLeft = Right & Left, // 100000
    };

    void acceptSelection();
    int boundsLeft(int newTopLeftX, const bool mouse = true);
    int boundsRight(int newTopLeftX, const bool mouse = true);
    int boundsUp(int newTopLeftY, const bool mouse = true);
    int boundsDown(int newTopLeftY, const bool mouse = true);
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent*) override;
    void drawBottomHelpText(QPainter& painter);
    void drawDragHandles(QPainter& painter);
    void drawMagnifier(QPainter& painter);
    void drawMidHelpText(QPainter& painter);
    void drawSelectionSizeTooltip(QPainter& painter, bool dragHandlesVisible);
    void setBottomHelpText();
    void layoutBottomHelpText();
    void setMouseCursor(const QPointF& pos);
    MouseState mouseLocation(const QPointF& pos);

    static const int handleRadiusMouse;
    static const int handleRadiusTouch;
    static const qreal increaseDragAreaFactor;
    static const int minSpacingBetweenHandles;
    static const int borderDragAreaSize;

    static const int selectionSizeThreshold;

    static const int selectionBoxPaddingX;
    static const int selectionBoxPaddingY;
    static const int selectionBoxMarginY;

    static const int bottomHelpMaxLength = 6;
    static bool bottomHelpTextPrepared;
    static const int bottomHelpBoxPaddingX;
    static const int bottomHelpBoxPaddingY;
    static const int bottomHelpBoxPairSpacing;
    static const int bottomHelpBoxMarginBottom;
    static const int midHelpTextFontSize;

    static const int magnifierLargeStep;

    static const int magZoom;
    static const int magPixels;
    static const int magOffset;

    QColor mMaskColor;
    QColor mStrokeColor;
    QColor mCrossColor;
    QColor mLabelBackgroundColor;
    QColor mLabelForegroundColor;
    QRect mSelection;
    QPointF mStartPos;
    QPointF mInitialTopLeft;
    QString mMidHelpText;
    QFont mMidHelpTextFont;
    std::pair<QStaticText, std::vector<QStaticText>> mBottomHelpText[bottomHelpMaxLength];
    QFont mBottomHelpTextFont;
    QRect mBottomHelpBorderBox;
    QPoint mBottomHelpContentPos;
    int mBottomHelpGridLeftWidth;
    MouseState mMouseDragState;
    QMap<ComparableQPoint, QImage> mImages;
    QVector<QPair<QRect, qreal>> mRectToDpr;
    QPixmap mPixmap;
    qreal devicePixelRatio;
    qreal devicePixelRatioI;
    QPointF mMousePos;
    bool mMagnifierAllowed;
    bool mShowMagnifier;
    bool mToggleMagnifier;
    bool mReleaseToCapture;
    bool mRememberRegion;
    bool mDisableArrowKeys;
    QRect mPrimaryScreenGeo;
    int mbottomHelpLength;
    QRegion mScreenRegion;

    // Midpoints of handles
    QVector<QPointF> mHandlePositions = QVector<QPointF> {8};
    // Radius of handles is either handleRadiusMouse or handleRadiusTouch
    int mHandleRadius;

Q_SIGNALS:

    void grabDone(const QPixmap &thePixmap);
    void grabCancelled();

private:

    QMap<ComparableQPoint, ComparableQPoint> computeCoordinatesAfterScaling(QMap<ComparableQPoint, QPair<qreal, QSize>> outputsRect);

    void preparePaint();
};

#endif // QUICKEDITOR_H
