/*
 *  SPDX-FileCopyrightText: 2018 Ambareesh "Amby" Balaji <ambareeshbalaji@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef QUICKEDITOR_H
#define QUICKEDITOR_H

#include <QMap>
#include <QStaticText>
#include <QWidget>

class ComparableQPoint;
class QKeyEvent;
class QMouseEvent;
class QPainter;

namespace KWayland::Client
{
class PlasmaShell;
}

class QuickEditor : public QWidget
{
    Q_OBJECT

public:
    QuickEditor(const QMap<const QScreen *, QImage> &images, KWayland::Client::PlasmaShell *plasmashell, QWidget *parent = nullptr);

Q_SIGNALS:
    void grabDone(const QPixmap &thePixmap);
    void grabCancelled();

private:
    enum MouseState : short {
        None = 0, // 0000
        Inside = 1 << 0, // 0001
        Outside = 1 << 1, // 0010
        TopLeft = 5, // 101
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
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *) override;
    void drawBottomCaptureInstructions(QPainter &painter);
    void drawDragHandles(QPainter &painter);
    void drawMagnifier(QPainter &painter);
    void drawMidCaptureInstructions(QPainter &painter);
    void preparePaint();
    void createPixmapFromScreens();
    void setGeometryToScreenPixmap(KWayland::Client::PlasmaShell *plasmashell);
    void drawSelectionSizeTooltip(QPainter &painter, bool dragHandlesVisible);
    void setBottomCaptureInstructions();
    void layoutBottomCaptureInstructions();
    void setMouseCursor(const QPointF &pos);
    MouseState mouseLocation(const QPointF &pos);

    static QMap<ComparableQPoint, ComparableQPoint> computeCoordinatesAfterScaling(const QMap<ComparableQPoint, QPair<qreal, QSize>> &outputsRect);

    static const int handleRadiusMouse;
    static const int handleRadiusTouch;
    static const qreal increaseDragAreaFactor;
    static const int minSpacingBetweenHandles;
    static const int borderDragAreaSize;

    static const int selectionSizeThreshold;

    static const int selectionBoxPaddingX;
    static const int selectionBoxPaddingY;
    static const int selectionBoxMarginY;

    static const int bottomCaptureInstructionLength = 6;
    static bool bottomCaptureInstructionPrepared;
    static const int bottomCaptureInstructionBoxPaddingX;
    static const int bottomCaptureInstructionBoxPaddingY;
    static const int bottomCaptureInstructionBoxPairSpacing;
    static const int bottomCaptureInstructionBoxMarginBottom;
    static const int midCaptureInstructionFontSize;

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
    QString mMidCaptureInstruction;
    QFont mMidCaptureInstructionFont;
    std::pair<QStaticText, std::vector<QStaticText>> mBottomCaptureInstructions[bottomCaptureInstructionLength];
    QFont mBottomCaptureInstructionFont;
    QRect mBottomCaptureInstructionBorderBox;
    QPoint mBottomCaptureInstructionContentPos;
    int mBottomCaptureInstructionGridLeftWidth;
    MouseState mMouseDragState;
    QMap<const QScreen *, QImage> mImages;
    QMap<const QScreen *, qreal> mScreenToDpr;
    QPixmap mPixmap;
    qreal devicePixelRatio;
    qreal devicePixelRatioI;
    QPointF mMousePos;
    bool mMagnifierAllowed;
    bool mShowMagnifier;
    bool mToggleMagnifier;
    bool mReleaseToCapture;
    bool mShowCaptureInstructions;
    bool mDisableArrowKeys;
    int mbottomCaptureInstructionsLength;
    QRect mScreensRect;

    // Midpoints of handles
    QVector<QPointF> mHandlePositions = QVector<QPointF>{8};
    // Radius of handles is either handleRadiusMouse or handleRadiusTouch
    int mHandleRadius;
};

#endif // QUICKEDITOR_H
