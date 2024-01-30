/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "AnnotationDocument.h"
#include <QMatrix4x4>
#include <QQuickPaintedItem>

class QPainter;

/**
 * This is a QML item which paints its correspondent AnnotationDocument or a sub-part of it,
 * depending from viewportRect and zoom.
 * This is also managing all the input which will add the annotations.
 */
class AnnotationViewport : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QRectF viewportRect READ viewportRect WRITE setViewportRect NOTIFY viewportRectChanged)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(AnnotationDocument *document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(QPointF hoverPosition READ hoverPosition NOTIFY hoverPositionChanged)
    Q_PROPERTY(bool hovered READ isHovered NOTIFY hoveredChanged)
    Q_PROPERTY(QPointF pressPosition READ pressPosition NOTIFY pressPositionChanged)
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged)
    Q_PROPERTY(bool anyPressed READ isAnyPressed NOTIFY anyPressedChanged)
    Q_PROPERTY(QPainterPath hoveredMousePath READ hoveredMousePath NOTIFY hoveredMousePathChanged)
    Q_PROPERTY(QMatrix4x4 localToDocument READ localToDocument NOTIFY localToDocumentChanged)
    Q_PROPERTY(QMatrix4x4 documentToLocal READ documentToLocal NOTIFY documentToLocalChanged)

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

    QPointF hoverPosition() const;

    bool isHovered() const;

    QPointF pressPosition() const;

    bool isPressed() const;

    bool isAnyPressed() const;

    QPainterPath hoveredMousePath() const;

    /**
     * Matrix for transforming from the local coordinate system to the document coordinate system.
     * Using QMatrix4x4 because QTransform doesn't work with QML.
     */
    QMatrix4x4 localToDocument() const;

    /**
     * Matrix for transforming from the document coordinate system to the local coordinate system.
     * Using QMatrix4x4 because QTransform doesn't work with QML.
     */
    QMatrix4x4 documentToLocal() const;

Q_SIGNALS:
    void viewportRectChanged();
    void documentChanged();
    void zoomChanged();
    void hoverPositionChanged();
    void hoveredChanged();
    void pressPositionChanged();
    void pressedChanged();
    void anyPressedChanged();
    void hoveredMousePathChanged();
    void localToDocumentChanged();
    void documentToLocalChanged();

protected:
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    bool shouldIgnoreInput() const;
    void setHoverPosition(const QPointF &point);
    void setHovered(bool hovered);
    void setPressPosition(const QPointF &point);
    void setPressed(bool pressed);
    void setAnyPressed();
    void setHoveredMousePath(const QPainterPath &path);
    void updateTransforms();
    // Repaint rect from document coordinate system or whole viewport if empty.
    void repaintDocument(const QRectF &documentRect);
    // Repaint rect from document coordinate system or nothing if empty.
    void repaintDocumentRect(const QRectF &documentRect);
    void setCursorForToolType();

    QRectF m_viewportRect;
    qreal m_zoom = 1.0;
    QPointer<AnnotationDocument> m_document;
    QPointF m_localHoverPosition;
    QPointF m_localPressPosition;
    QPointF m_lastDocumentPressPos;
    QMatrix4x4 m_localToDocument;
    QMatrix4x4 m_documentToLocal;
    bool m_isHovered = false;
    bool m_isPressed = false;
    bool m_allowDraggingSelection = false;
    bool m_acceptKeyReleaseEvents = false;
    QPainterPath m_hoveredMousePath;
    static QList<AnnotationViewport *> s_viewportInstances;
};
