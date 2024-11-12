/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "AnnotationDocument.h"
#include <QMatrix4x4>
#include <QQuickItem>
#include <qqmlregistration.h>

class QPainter;

/**
 * This is a QML item which paints its correspondent AnnotationDocument or a sub-part of it,
 * depending from viewportRect and zoom.
 * This is also managing all the input which will add the annotations.
 */
class AnnotationViewport : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QRectF viewportRect READ viewportRect WRITE setViewportRect NOTIFY viewportRectChanged)
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
    ~AnnotationViewport() noexcept override;

    QRectF viewportRect() const;
    void setViewportRect(const QRectF &rect);

    AnnotationDocument *document() const;
    void setDocument(AnnotationDocument *doc);

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
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void itemChange(ItemChange, const ItemChangeData &) override;

private:
    bool shouldIgnoreInput() const;
    void setHoverPosition(const QPointF &point);
    void setHovered(bool hovered);
    void setPressPosition(const QPointF &point);
    void setPressed(bool pressed);
    void setAnyPressed();
    void setHoveredMousePath(const QPainterPath &path);
    void updateTransforms();
    void setCursorForToolType();

    QRectF m_viewportRect;
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
    bool m_repaintBaseImage = true;
    bool m_repaintAnnotations = true;
    static QList<AnnotationViewport *> s_viewportInstances;
};
