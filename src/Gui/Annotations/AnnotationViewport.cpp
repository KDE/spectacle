/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "AnnotationViewport.h"
#include "AnnotationDocument.h"
#include "EditAction.h"

#include <QCursor>
#include <QPainter>
#include <QScreen>

AnnotationViewport::AnnotationViewport(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setFlag(ItemIsFocusScope);
    setAcceptedMouseButtons(Qt::LeftButton);
}

AnnotationViewport::~AnnotationViewport() noexcept
{
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
    connect(doc, &AnnotationDocument::repaintNeeded, this, &AnnotationViewport::onRepaintNeeded);
    connect(doc->tool(), &AnnotationTool::typeChanged,
            this, &AnnotationViewport::setCursorForToolType);
    Q_EMIT documentChanged();
    update();
}

void AnnotationViewport::paint(QPainter *painter)
{
    if (!m_document || m_viewportRect.isEmpty()) {
        return;
    }

    m_document->paint(painter, m_viewportRect, m_zoom);
}

void AnnotationViewport::mousePressEvent(QMouseEvent *event)
{
    if (shouldIgnoreInput() || event->buttons() & ~acceptedMouseButtons()) {
        QQuickItem::mousePressEvent(event);
        return;
    }

    qreal zoom = this->zoom();
    QPointF scaledMousePos = event->localPos() / zoom;
    m_lastScaledViewPressPos = m_viewportRect.topLeft() / zoom + scaledMousePos;
    auto toolType = m_document->tool()->type();
    auto saWrapper = m_document->selectedActionWrapper();

    // commit changes to selected text actions
    if (toolType != AnnotationDocument::None && saWrapper->type() == AnnotationDocument::Text) {
        saWrapper->commitChanges();
    }

    if (toolType == AnnotationDocument::ChangeAction) {
        m_document->selectAction(m_lastScaledViewPressPos);
        // Immediately commit to immediately pop an action copy up in the z order
        saWrapper->commitChanges();
    } else {
        m_document->beginAction(m_lastScaledViewPressPos);
    }

    m_allowDraggingSelectedAction = toolType == AnnotationDocument::ChangeAction
                                 && saWrapper->type() != AnnotationDocument::None;
    m_lastSelectedActionVisualGeometry = saWrapper->visualGeometry();

    Q_EMIT pressed(scaledMousePos);
    event->accept();
}

void AnnotationViewport::mouseMoveEvent(QMouseEvent *event)
{
    if (shouldIgnoreInput() || event->buttons() & ~acceptedMouseButtons()) {
        QQuickItem::mouseMoveEvent(event);
        return;
    }

    auto toolType = m_document->tool()->type();
    qreal zoom = this->zoom();
    QPointF scaledMousePos = event->localPos() / zoom;
    QRectF scaledViewRect = {m_viewportRect.topLeft() / zoom,
                                 m_viewportRect.size() / zoom};
    QPointF scaledViewMousePos = scaledViewRect.topLeft() + scaledMousePos;

    auto saWrapper = m_document->selectedActionWrapper();
    if (toolType == AnnotationDocument::ChangeAction
        && saWrapper->type() != AnnotationDocument::None
        && m_allowDraggingSelectedAction
    ) {
        auto visualGeometry = saWrapper->visualGeometry();
        QPointF posDiff = scaledViewMousePos - m_lastScaledViewPressPos;
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
        m_document->continueAction(scaledViewMousePos, options);
    }

    event->accept();
}

void AnnotationViewport::mouseReleaseEvent(QMouseEvent *event)
{
    if (shouldIgnoreInput() || event->buttons() & ~acceptedMouseButtons()) {
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

bool AnnotationViewport::shouldIgnoreInput() const
{
    return !isEnabled() || !m_document || m_document->tool()->type() == AnnotationDocument::None;
}

void AnnotationViewport::onRepaintNeeded(const QRectF &area)
{
    auto scaledArea = QRectF(area.x() * m_zoom, area.y() * m_zoom, area.width() * m_zoom, area.height() * m_zoom);
    if (area.isEmpty()) {
        update();
    } else if (!m_viewportRect.intersects(scaledArea)) {
        return;
    } else {
        update(scaledArea.translated(-m_viewportRect.topLeft() * m_zoom).toAlignedRect());
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
