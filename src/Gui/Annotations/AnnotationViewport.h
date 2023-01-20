/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "AnnotationDocument.h"
#include <QQuickPaintedItem>

class QPainter;

/**
 * This is a QML item which paints its correspondent AnnotationDocument or a sub-part of it,
 * depending from viewportRect and zoom.
 * This is also managing all the input which will do the edit actions.
 */
class AnnotationViewport : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QRectF viewportRect READ viewportRect WRITE setViewportRect NOTIFY viewportRectChanged)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(AnnotationDocument *document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(QPointF pressPosition READ pressPosition NOTIFY pressPositionChanged)
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged)
    Q_PROPERTY(bool anyPressed READ isAnyPressed NOTIFY anyPressedChanged)

public:
    explicit AnnotationViewport(QQuickItem *parent = nullptr);
    ~AnnotationViewport() override;

    QRectF viewportRect() const;
    void setViewportRect(const QRectF &rect);

    void setZoom(qreal zoom);
    qreal zoom() const;

    AnnotationDocument *document() const;
    void setDocument(AnnotationDocument *doc);

    void paint(QPainter *painter) override;

    QPointF pressPosition() const;

    bool isPressed() const;

    bool isAnyPressed() const;

Q_SIGNALS:
    void viewportRectChanged();
    void documentChanged();
    void zoomChanged();
    void pressPositionChanged();
    void pressedChanged();
    void anyPressedChanged();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    bool shouldIgnoreInput() const;
    void setPressPosition(const QPointF &point);
    void setPressed(bool pressed);
    void setAnyPressed();
    void onRepaintNeeded(const QRectF &area);
    void setCursorForToolType();

    QRectF m_viewportRect;
    qreal m_zoom = 1.0;
    QPointer<AnnotationDocument> m_document;
    QPointF m_lastScaledViewPressPos;
    QPointF m_pressPosition;
    QRectF m_lastSelectedActionVisualGeometry;
    bool m_isPressed = false;
    bool m_allowDraggingSelectedAction = false;
    bool m_acceptKeyReleaseEvents = false;
};
