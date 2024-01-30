/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "AnnotationViewport.h"

#include <QCursor>
#include <QPainter>
#include <QScreen>
#include <utility>

QList<AnnotationViewport *> AnnotationViewport::s_viewportInstances = {};
static bool s_synchronizingAnyPressed = false;
static bool s_isAnyPressed = false;

AnnotationViewport::AnnotationViewport(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    s_viewportInstances.append(this);
    setFlag(ItemIsFocusScope);
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
}

AnnotationViewport::~AnnotationViewport() noexcept
{
    setPressed(false);
    s_viewportInstances.removeOne(this);
}

QRectF AnnotationViewport::viewportRect() const
{
    return m_viewportRect;
}

void AnnotationViewport::setViewportRect(const QRectF &rect)
{
    if (rect == m_viewportRect) {
        return;
    }
    m_viewportRect = rect;
    Q_EMIT viewportRectChanged();
    updateTransforms();
    update();
}

void AnnotationViewport::setZoom(qreal zoom)
{
    if (zoom == m_zoom || qFuzzyIsNull(zoom)) {
        return;
    }

    m_zoom = zoom;
    Q_EMIT zoomChanged();
    updateTransforms();
    update();
}

qreal AnnotationViewport::zoom() const
{
    return m_zoom;
}

AnnotationDocument *AnnotationViewport::document() const
{
    return m_document;
}

void AnnotationViewport::setDocument(AnnotationDocument *doc)
{
    if (m_document == doc) {
        return;
    }

    if (m_document) {
        disconnect(m_document, nullptr, this, nullptr);
    }

    m_document = doc;
    connect(doc, &AnnotationDocument::repaintNeeded, this, &AnnotationViewport::repaintDocument);
    connect(doc->tool(), &AnnotationTool::typeChanged, this, &AnnotationViewport::setCursorForToolType);
    Q_EMIT documentChanged();
    update();
}

QPointF AnnotationViewport::hoverPosition() const
{
    return m_localHoverPosition;
}

void AnnotationViewport::setHoverPosition(const QPointF &point)
{
    if (m_localHoverPosition == point) {
        return;
    }
    m_localHoverPosition = point;
    Q_EMIT hoverPositionChanged();
}

bool AnnotationViewport::isHovered() const
{
    return m_isHovered;
}

void AnnotationViewport::setHovered(bool hovered)
{
    if (m_isHovered == hovered) {
        return;
    }

    m_isHovered = hovered;
    Q_EMIT hoveredChanged();
}

void setHovered(bool hovered);

QPointF AnnotationViewport::pressPosition() const
{
    return m_localPressPosition;
}

void AnnotationViewport::setPressPosition(const QPointF &point)
{
    if (m_localPressPosition == point) {
        return;
    }
    m_localPressPosition = point;
    Q_EMIT pressPositionChanged();
}

bool AnnotationViewport::isPressed() const
{
    return m_isPressed;
}

void AnnotationViewport::setPressed(bool pressed)
{
    if (m_isPressed == pressed) {
        return;
    }

    m_isPressed = pressed;
    Q_EMIT pressedChanged();
    setAnyPressed();
}

bool AnnotationViewport::isAnyPressed() const
{
    return s_isAnyPressed;
}

void AnnotationViewport::setAnyPressed()
{
    if (s_synchronizingAnyPressed || s_isAnyPressed == m_isPressed) {
        return;
    }
    s_synchronizingAnyPressed = true;
    // If pressed is true, anyPressed is guaranteed to be true.
    // If pressed is false, anyPressed may still be true if another viewport is pressed.
    const bool oldAnyPressed = s_isAnyPressed;
    if (m_isPressed) {
        s_isAnyPressed = m_isPressed;
    } else {
        for (const auto viewport : std::as_const(s_viewportInstances)) {
            s_isAnyPressed = viewport->m_isPressed;
            if (s_isAnyPressed) {
                break;
            }
        }
    }
    // Don't emit if s_isAnyPressed still hasn't changed
    if (oldAnyPressed != s_isAnyPressed) {
        for (const auto viewport : std::as_const(s_viewportInstances)) {
            Q_EMIT viewport->anyPressedChanged();
        }
    }
    s_synchronizingAnyPressed = false;
}

QPainterPath AnnotationViewport::hoveredMousePath() const
{
    return m_hoveredMousePath;
}

void AnnotationViewport::setHoveredMousePath(const QPainterPath &path)
{
    if (path == m_hoveredMousePath) {
        return;
    }
    m_hoveredMousePath = path;
    Q_EMIT hoveredMousePathChanged();
}

QMatrix4x4 AnnotationViewport::localToDocument() const
{
    return m_localToDocument;
}

void AnnotationViewport::updateTransforms()
{
    QMatrix4x4 localToDocument;
    // translate() is affected by existing scales,
    // but scale() does not affect existing translations
    localToDocument.scale(1 / m_zoom, 1 / m_zoom);
    localToDocument.translate(m_viewportRect.x(), m_viewportRect.y());

    if (m_localToDocument != localToDocument) {
        m_localToDocument = localToDocument;
        Q_EMIT localToDocumentChanged();
    }
    QMatrix4x4 documentToLocal;
    documentToLocal.scale(m_zoom, m_zoom);
    documentToLocal.translate(-m_viewportRect.x(), -m_viewportRect.y());
    if (m_documentToLocal != documentToLocal) {
        m_documentToLocal = documentToLocal;
        Q_EMIT documentToLocalChanged();
    }
}

QMatrix4x4 AnnotationViewport::documentToLocal() const
{
    return m_documentToLocal;
}

void AnnotationViewport::paint(QPainter *painter)
{
    if (!m_document || m_viewportRect.isEmpty()) {
        return;
    }

    m_document->paint(painter, m_viewportRect, m_zoom);
}

void AnnotationViewport::hoverEnterEvent(QHoverEvent *event)
{
    if (shouldIgnoreInput()) {
        QQuickItem::hoverEnterEvent(event);
        return;
    }
    setHoverPosition(event->position());
    setHovered(true);
}

void AnnotationViewport::hoverMoveEvent(QHoverEvent *event)
{
    if (shouldIgnoreInput()) {
        QQuickItem::hoverMoveEvent(event);
        return;
    }
    setHoverPosition(event->position());

    if (m_document->tool()->type() == AnnotationTool::SelectTool) {
        // Without as_const, QMatrix4x4 will go to General mode with operator()(row, column).
        // keep margin the same number of pixels regardless of zoom level.
        auto margin = 4 * std::as_const(m_documentToLocal)(0, 0); // m11/x scale
        QRectF forgivingRect{event->position(), QSizeF{0, 0}};
        forgivingRect.adjust(-margin, -margin, margin, margin);
        if (auto item = m_document->itemAt(m_localToDocument.mapRect(forgivingRect))) {
            auto &geometry = std::get<Traits::Geometry::Opt>(item->traits());
            setHoveredMousePath(geometry->mousePath);
        } else {
            setHoveredMousePath({});
        }
    } else {
        setHoveredMousePath({});
    }
}

void AnnotationViewport::hoverLeaveEvent(QHoverEvent *event)
{
    if (shouldIgnoreInput()) {
        QQuickItem::hoverLeaveEvent(event);
        return;
    }
    setHovered(false);
}

void AnnotationViewport::mousePressEvent(QMouseEvent *event)
{
    if (shouldIgnoreInput() || event->buttons() & ~acceptedMouseButtons()) {
        QQuickItem::mousePressEvent(event);
        return;
    }

    auto toolType = m_document->tool()->type();
    auto wrapper = m_document->selectedItemWrapper();
    auto pressPos = event->position();
    m_lastDocumentPressPos = m_localToDocument.map(pressPos);

    if (toolType == AnnotationTool::SelectTool) {
        auto margin = 4 * std::as_const(m_documentToLocal)(0, 0); // m11/x scale
        QRectF forgivingRect{pressPos, QSizeF{0, 0}};
        forgivingRect.adjust(-margin, -margin, margin, margin);
        m_document->selectItem(m_localToDocument.mapRect(forgivingRect));
    } else {
        wrapper->commitChanges();
        m_document->beginItem(m_lastDocumentPressPos);
    }

    m_allowDraggingSelection = toolType == AnnotationTool::SelectTool && wrapper->hasSelection();

    setHoveredMousePath({});
    setPressPosition(pressPos);
    setPressed(true);
    event->accept();
}

void AnnotationViewport::mouseMoveEvent(QMouseEvent *event)
{
    if (shouldIgnoreInput() || event->buttons() & ~acceptedMouseButtons()) {
        QQuickItem::mouseMoveEvent(event);
        return;
    }

    auto toolType = m_document->tool()->type();
    auto mousePos = event->position();
    auto wrapper = m_document->selectedItemWrapper();
    if (toolType == AnnotationTool::SelectTool && wrapper->hasSelection() && m_allowDraggingSelection) {
        auto documentMousePos = m_localToDocument.map(event->position());
        auto dx = documentMousePos.x() - m_lastDocumentPressPos.x();
        auto dy = documentMousePos.y() - m_lastDocumentPressPos.y();
        wrapper->transform(dx, dy);
    } else if (toolType > AnnotationTool::SelectTool) {
        using ContinueOptions = AnnotationDocument::ContinueOptions;
        using ContinueOption = AnnotationDocument::ContinueOption;
        ContinueOptions options;
        if (event->modifiers() & Qt::ShiftModifier) {
            options |= ContinueOption::SnapAngle;
        }
        if (event->modifiers() & Qt::ControlModifier) {
            options |= ContinueOption::CenterResize;
        }
        m_document->continueItem(m_localToDocument.map(mousePos), options);
    }

    setHoveredMousePath({});
    event->accept();
}

void AnnotationViewport::mouseReleaseEvent(QMouseEvent *event)
{
    if (shouldIgnoreInput() || event->buttons() & ~acceptedMouseButtons()) {
        QQuickItem::mouseReleaseEvent(event);
        return;
    }

    m_document->finishItem();

    auto toolType = m_document->tool()->type();
    auto wrapper = m_document->selectedItemWrapper();
    auto selectedOptions = wrapper->options();
    if (!selectedOptions.testFlag(AnnotationTool::TextOption) //
        && !m_document->isCurrentItemValid()) {
        m_document->popCurrentItem();
    } else if (toolType == AnnotationTool::SelectTool && wrapper->hasSelection()) {
        wrapper->commitChanges();
    } else if (!selectedOptions.testFlag(AnnotationTool::TextOption)) {
        m_document->deselectItem();
    }

    setPressed(false);
    event->accept();
}

void AnnotationViewport::keyPressEvent(QKeyEvent *event)
{
    // For some reason, events are already accepted when they arrive.
    QQuickItem::keyPressEvent(event);
    if (shouldIgnoreInput()) {
        m_acceptKeyReleaseEvents = false;
        return;
    }

    const auto wrapper = m_document->selectedItemWrapper();
    const auto selectedOptions = wrapper->options();
    const auto toolType = m_document->tool()->type();
    if (wrapper->hasSelection()) {
        if (event->matches(QKeySequence::Cancel)) {
            m_document->deselectItem();
            if (!m_document->isCurrentItemValid()) {
                m_document->popCurrentItem();
            }
            event->accept();
        } else if (event->matches(QKeySequence::Delete) //
                   && toolType == AnnotationTool::SelectTool //
                   && (!selectedOptions.testFlag(AnnotationTool::TextOption)
                       || wrapper->text().isEmpty())) {
            // Only use delete shortcut when not using the text tool.
            // We don't want users trying to delete text to accidentally delete the item.
            m_document->deleteSelectedItem();
            event->accept();
        }
    }
    m_acceptKeyReleaseEvents = event->isAccepted();
}

void AnnotationViewport::keyReleaseEvent(QKeyEvent *event)
{
    // For some reason, events are already accepted when they arrive.
    if (shouldIgnoreInput()) {
        QQuickItem::keyReleaseEvent(event);
    } else {
        event->setAccepted(m_acceptKeyReleaseEvents);
    }
    m_acceptKeyReleaseEvents = false;
}

bool AnnotationViewport::shouldIgnoreInput() const
{
    return !isEnabled() || !m_document || m_document->tool()->type() == AnnotationTool::NoTool;
}

void AnnotationViewport::repaintDocument(const QRectF &documentRect)
{
    if (documentRect.isEmpty()) {
        update();
        return;
    }

    repaintDocumentRect(documentRect);
}

void AnnotationViewport::repaintDocumentRect(const QRectF &documentRect)
{
    auto localRect = m_documentToLocal.mapRect(documentRect).toAlignedRect();
    // intersects returns false if either rect is empty
    if (boundingRect().intersects(localRect)) {
        update(localRect);
    }
}

void AnnotationViewport::setCursorForToolType()
{
    if (m_document && isEnabled()) {
        if (m_document->tool()->type() == AnnotationTool::SelectTool) {
            setCursor(Qt::ArrowCursor);
        } else {
            setCursor(Qt::CrossCursor);
        }
    } else {
        unsetCursor();
    }
}

#include <moc_AnnotationViewport.cpp>
