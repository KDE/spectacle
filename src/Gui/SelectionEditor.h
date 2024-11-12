/*
 *  SPDX-FileCopyrightText: 2018 Ambareesh "Amby" Balaji <ambareeshbalaji@gmail.com>
 *  SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef SELECTIONEDITOR_H
#define SELECTIONEDITOR_H

#include "ExportManager.h"

#include <memory>

#include <QQmlEngine>

#include "Selection.h"

class QHoverEvent;
class QKeyEvent;
class QMouseEvent;
class QQuickItem;
class Selection;
class SelectionEditorPrivate;

/**
 * This class is used to set the selected rectangle capture region,
 * get information related to it and handle input from a capture window.
 */
class SelectionEditor : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(Selection *selection READ selection CONSTANT FINAL)
    Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio NOTIFY devicePixelRatioChanged FINAL)
    Q_PROPERTY(QRectF screensRect READ screensRect NOTIFY screensRectChanged FINAL)
    Q_PROPERTY(Location dragLocation READ dragLocation NOTIFY dragLocationChanged FINAL)
    Q_PROPERTY(QRectF handlesRect READ handlesRect NOTIFY handlesRectChanged FINAL)
    Q_PROPERTY(QPointF mousePosition READ mousePosition NOTIFY mousePositionChanged)
    /// Whether or not to show the magnifier.
    Q_PROPERTY(bool showMagnifier READ showMagnifier NOTIFY showMagnifierChanged FINAL)
    /// The location that the magnifier is looking at,
    /// not necessarily the location of the magnifier.
    Q_PROPERTY(Location magnifierLocation READ magnifierLocation NOTIFY magnifierLocationChanged FINAL)

public:
    /// Locations in relation to the current selection
    enum Location : short {
        None = 0,
        Outside,
        // clang-format off
        TopLeft,    Top,    TopRight,
        Left,       Inside, Right,
        BottomLeft, Bottom, BottomRight,
        // clang-format on
        FollowMouse = Outside, // Semantic alias for the magnifier
    };
    Q_ENUM(Location)

    static SelectionEditor *instance();

    Selection *selection() const;

    qreal devicePixelRatio() const;

    QRectF screensRect() const;

    qreal screensWidth() const;
    qreal screensHeight() const;

    Location dragLocation() const;

    QRectF handlesRect() const;

    QPointF mousePosition() const;

    bool showMagnifier() const;

    Location magnifierLocation() const;

    Q_SLOT bool acceptSelection(ExportManager::Actions actions = {});

    void reset();

    static SelectionEditor *create(QQmlEngine *engine, QJSEngine *)
    {
        auto inst = instance();
        Q_ASSERT(inst);
        Q_ASSERT(inst->thread() == engine->thread());
        QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
        return inst;
    }

Q_SIGNALS:
    void devicePixelRatioChanged();
    void screensRectChanged();
    void dragLocationChanged();
    void handlesRectChanged();
    void mousePositionChanged();
    void showMagnifierChanged();
    void magnifierLocationChanged();

    void accepted(const QRectF &rect, const ExportManager::Actions &actions);

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
    friend class SelectionEditorSingleton;
    explicit SelectionEditor(QObject *parent = nullptr);
    const std::unique_ptr<SelectionEditorPrivate> d;
};

#endif // SELECTIONEDITOR_H
