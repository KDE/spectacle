/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "AnnotationViewport.h"
#include "EditAction.h"

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
    update();
}

void AnnotationViewport::setZoom(qreal zoom)
{
    if (zoom == m_zoom) {
        return;
    }

    m_zoom = zoom;
    Q_EMIT zoomChanged();
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
    connect(doc->tool(), &AnnotationTool::typeChanged,
            this, &AnnotationViewport::setCursorForToolType);
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
    if (shouldIgnoreInput() || event->buttons() & ~acceptedMouseButtons() || event->buttons() == Qt::NoButton) {
        QQuickItem::mousePressEvent(event);
        return;
    }

    m_lastDocumentPressPos = toDocumentPoint(event->position());
    auto toolType = m_document->tool()->type();
    auto saWrapper = m_document->selectedActionWrapper();

    // commit changes to selected text actions
    if (toolType != AnnotationDocument::None && saWrapper->type() == AnnotationDocument::Text) {
        saWrapper->commitChanges();
    }

    if (toolType == AnnotationDocument::ChangeAction) {
        m_document->selectAction(m_lastDocumentPressPos);
        // Immediately commit to immediately pop an action copy up in the z order
        saWrapper->commitChanges();
    } else {
        m_document->beginAction(m_lastDocumentPressPos);
    }

    m_allowDraggingSelectedAction = toolType == AnnotationDocument::ChangeAction
                                 && saWrapper->type() != AnnotationDocument::None;
    m_lastSelectedActionVisualGeometry = saWrapper->visualGeometry();

    setPressPosition(event->position());
    setPressed(true);
    event->accept();
}

void AnnotationViewport::mouseMoveEvent(QMouseEvent *event)
{
    if (shouldIgnoreInput() || event->buttons() & ~acceptedMouseButtons() || event->buttons() == Qt::NoButton) {
        QQuickItem::mouseMoveEvent(event);
        return;
    }

    auto toolType = m_document->tool()->type();
    QPointF documentMousePos = toDocumentPoint(event->position());

    auto saWrapper = m_document->selectedActionWrapper();
    if (toolType == AnnotationDocument::ChangeAction
        && saWrapper->type() != AnnotationDocument::None
        && m_allowDraggingSelectedAction
    ) {
        auto visualGeometry = saWrapper->visualGeometry();
        QPointF posDiff = documentMousePos - m_lastDocumentPressPos;
        visualGeometry.moveTo(m_lastSelectedActionVisualGeometry.topLeft() + posDiff);
        saWrapper->setVisualGeometry(visualGeometry);
    } else if (toolType != AnnotationDocument::None) {
        using ContinueOptions = AnnotationDocument::ContinueOptions;
        using ContinueOption = AnnotationDocument::ContinueOption;
        ContinueOptions options;
        if (event->modifiers() & Qt::ShiftModifier) {
            options |= ContinueOption::SnapAngle;
        }
        if (event->modifiers() & Qt::ControlModifier) {
            options |= ContinueOption::CenterResize;
        }
        m_document->continueAction(documentMousePos, options);
    }

    setPressPosition(event->position());
    event->accept();
}

void AnnotationViewport::mouseReleaseEvent(QMouseEvent *event)
{
    if (shouldIgnoreInput() || event->button() & ~acceptedMouseButtons()) {
        QQuickItem::mouseReleaseEvent(event);
        return;
    }

    m_document->finishAction();

    auto toolType = m_document->tool()->type();
    auto saWrapper = m_document->selectedActionWrapper();
    auto saType = saWrapper->type();
    if (m_document->isLastActionInvalid() && saType != AnnotationDocument::Text) {
        m_document->permanentlyDeleteLastAction();
    } else if (toolType == AnnotationDocument::ChangeAction && saType != AnnotationDocument::None) {
        saWrapper->commitChanges();
        update();
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

    const auto saWrapper = m_document->selectedActionWrapper();
    const auto saType = saWrapper->type();
    const auto toolType = m_document->tool()->type();
    const bool usingChangeTool = toolType == AnnotationDocument::ChangeAction;
    if (saType != AnnotationDocument::None) {
        if (event->matches(QKeySequence::Cancel)) {
            m_document->deselectAction();
            if (m_document->isLastActionInvalid()) {
                m_document->permanentlyDeleteLastAction();
            }
            event->accept();
        } else if (event->matches(QKeySequence::Delete)
            && usingChangeTool && saType != AnnotationDocument::Text) {
            // Only use delete shortcut when not using the text tool.
            // We don't want users trying to delete text to accidentally delete the action.
            m_document->deleteSelectedAction();
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

QPointF AnnotationViewport::toDocumentPoint(const QPointF &point) const
{
    return (point + m_viewportRect.topLeft()) / m_zoom;
}

QRectF AnnotationViewport::toLocalRect(const QRectF &rect) const
{
    return {(rect.topLeft() - m_viewportRect.topLeft()) * m_zoom,
            rect.size() * m_zoom};
}

bool AnnotationViewport::shouldIgnoreInput() const
{
    return !isEnabled() || !m_document || m_document->tool()->type() == AnnotationDocument::None;
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
    auto localRect = toLocalRect(documentRect).toAlignedRect();
    // intersects returns false if either rect is empty
    if (boundingRect().intersects(localRect)) {
        update(localRect);
    }
}

void AnnotationViewport::setCursorForToolType()
{
    if (m_document && isEnabled()) {
        if (m_document->tool()->type() == AnnotationDocument::ChangeAction) {
            setCursor(Qt::ArrowCursor);
        } else {
            setCursor(Qt::CrossCursor);
        }
    } else {
        unsetCursor();
    }
}

#include <moc_AnnotationViewport.cpp>
