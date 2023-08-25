/*
 *  SPDX-FileCopyrightText: 2018 Ambareesh "Amby" Balaji <ambareeshbalaji@gmail.com>
 *  SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef SELECTIONEDITOR_H
#define SELECTIONEDITOR_H

#include "Selection.h"
#include "CanvasImage.h"
#include "ExportManager.h"

#include <QMap>
#include <QPixmap>
#include <QQuickPaintedItem>

#include <memory>

class ComparableQPoint;
class QKeyEvent;
class QMouseEvent;
class QPainter;
class CaptureWindow;
class Selection;
class SelectionEditorPrivate;

/**
 * This class is used to set the selected rectangle capture region,
 * get information related to it and handle input from a capture window.
 */
class SelectionEditor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Selection *selection READ selection CONSTANT FINAL)
    Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio NOTIFY devicePixelRatioChanged FINAL)
    Q_PROPERTY(QRectF screensRect READ screensRect NOTIFY screensRectChanged FINAL)
    Q_PROPERTY(MouseLocation dragLocation READ dragLocation NOTIFY dragLocationChanged FINAL)
    Q_PROPERTY(QRectF handlesRect READ handlesRect NOTIFY handlesRectChanged FINAL)
    Q_PROPERTY(bool magnifierAllowed READ magnifierAllowed NOTIFY magnifierAllowedChanged FINAL)
    Q_PROPERTY(QPointF mousePosition READ mousePosition NOTIFY mousePositionChanged)

public:
    enum MouseLocation : short {
        None =          0b000000,
        Inside =        0b000001,
        Outside =       0b000010,
        TopLeft =       0b000101,
        Top =           0b010001,
        TopRight =      0b001001,
        Right =         0b100001,
        BottomRight =   0b000110,
        Bottom =        0b010010,
        BottomLeft =    0b001010,
        Left =          0b100010,
        TopLeftOrBottomRight = TopLeft & BottomRight, // 100
        TopRightOrBottomLeft = TopRight & BottomLeft, // 1000
        TopOrBottom = Top & Bottom, // 10000
        RightOrLeft = Right & Left, // 100000
    };
    Q_ENUM(MouseLocation)

    explicit SelectionEditor(QObject *parent = nullptr);

    static SelectionEditor *instance();

    Selection *selection() const;

    qreal devicePixelRatio() const;
    QRectF screensRect() const;

    int width() const;
    int height() const;

    MouseLocation dragLocation() const;

    QRectF handlesRect() const;

    bool magnifierAllowed() const;
    QPointF mousePosition() const;

    /**
     * Round value to be physically pixel perfect, based on the device pixel ratio.
     * Meant to be used with coordinates, line widths and shape sizes.
     * This is not meant to be used in QML.
     */
    qreal dprRound(qreal value, qreal dpr) const;
    qreal dprRound(qreal value) const;

    /**
     * Sets the images to use for each screen.
     * Each image should match a screen 1:1 in the order that they are listed
     * by QGuiApplication::screens().
     * Only used with CaptureModeModel::RectangularRegion
     */
    Q_SLOT void setScreenImages(const QVector<CanvasImage> &screenImages);
    QVector<CanvasImage> screenImages() const;

    Q_SLOT bool acceptSelection(ExportManager::Actions actions = {});

Q_SIGNALS:
    void devicePixelRatioChanged();
    void screensRectChanged();
    void dragLocationChanged();
    void handlesRectChanged();
    void screenImagesChanged();
    void magnifierAllowedChanged();
    void mousePositionChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QQuickItem *item, QKeyEvent *event);
    void keyReleaseEvent(QQuickItem *item, QKeyEvent *event);
    void hoverMoveEvent(QQuickItem *item, QHoverEvent *event);
    void mousePressEvent(QQuickItem *item, QMouseEvent *event);
    void mouseMoveEvent(QQuickItem *item, QMouseEvent *event);
    void mouseReleaseEvent(QQuickItem *item, QMouseEvent *event);
    void mouseDoubleClickEvent(QQuickItem *item, QMouseEvent *event);

private:
    const std::unique_ptr<SelectionEditorPrivate> d;
};

#endif // SELECTIONEDITOR_H
