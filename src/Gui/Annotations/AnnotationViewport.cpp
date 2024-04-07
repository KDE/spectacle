/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "AnnotationViewport.h"
#include "Geometry.h"

#include <QCursor>
#include <QPainter>
#include <QQuickWindow>
#include <QSGImageNode>
#include <QScreen>
#include <utility>

using G = Geometry;

QList<AnnotationViewport *> AnnotationViewport::s_viewportInstances = {};
static bool s_synchronizingAnyPressed = false;
static bool s_isAnyPressed = false;

class AnnotationViewportNode : public QSGNode
{
    QSGImageNode *m_baseImageNode;
    QSGImageNode *m_annotationsNode;

public:
    AnnotationViewportNode(QSGImageNode *baseImageNode, QSGImageNode *annotationsNode)
        : QSGNode()
        , m_baseImageNode(baseImageNode)
        , m_annotationsNode(annotationsNode)
    {
        baseImageNode->setOwnsTexture(true);
        appendChildNode(baseImageNode);
        annotationsNode->setOwnsTexture(true);
        appendChildNode(annotationsNode);
    }
    QSGImageNode *baseImageNode() const
    {
        return m_baseImageNode;
    }
    QSGImageNode *annotationsNode() const
    {
        return m_annotationsNode;
    }
};

AnnotationViewport::AnnotationViewport(QQuickItem *parent)
    : QQuickItem(parent)
{
    s_viewportInstances.append(this);
    setFlags({ItemIsFocusScope, ItemHasContents, ItemIsViewport, ItemObservesViewport});
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
    m_repaintBaseImage = true;
    m_repaintAnnotations = true;
    update();
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
    auto repaint = [this](AnnotationDocument::RepaintTypes types) {
        using RepaintType = AnnotationDocument::RepaintType;
        if (types.testFlag(RepaintType::BaseImage)) {
            m_repaintBaseImage = true;
        }
        if (types.testFlag(RepaintType::Annotations)) {
            m_repaintAnnotations = true;
        }
        update();
    };
    connect(doc, &AnnotationDocument::repaintNeeded, this, repaint);
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
    localToDocument.translate(m_viewportRect.x(), m_viewportRect.y());
    QMatrix4x4 documentToLocal;
    documentToLocal.translate(-m_viewportRect.x(), -m_viewportRect.y());
    if (m_document) {
        const auto canvasPos = m_document->canvasRect().topLeft();
        localToDocument.translate(canvasPos.x(), canvasPos.y());
        documentToLocal.translate(-canvasPos.x(), -canvasPos.y());
    }

    if (m_localToDocument != localToDocument) {
        m_localToDocument = localToDocument;
        Q_EMIT localToDocumentChanged();
    }
    if (m_documentToLocal != documentToLocal) {
        m_documentToLocal = documentToLocal;
        Q_EMIT documentToLocalChanged();
    }
}

QMatrix4x4 AnnotationViewport::documentToLocal() const
{
    return m_documentToLocal;
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
            auto &interactive = std::get<Traits::Interactive::Opt>(item->traits());
            setHoveredMousePath(interactive->path);
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
    if (shouldIgnoreInput() || event->buttons() & ~acceptedMouseButtons() || event->buttons() == Qt::NoButton) {
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
    if (shouldIgnoreInput() || event->buttons() & ~acceptedMouseButtons() || event->buttons() == Qt::NoButton) {
        QQuickItem::mouseMoveEvent(event);
        return;
    }

    auto tool = m_document->tool();
    auto mousePos = event->position();
    auto wrapper = m_document->selectedItemWrapper();
    if (tool->type() == AnnotationTool::SelectTool && wrapper->hasSelection() && m_allowDraggingSelection) {
        auto documentMousePos = m_localToDocument.map(event->position());
        auto dx = documentMousePos.x() - m_lastDocumentPressPos.x();
        auto dy = documentMousePos.y() - m_lastDocumentPressPos.y();
        wrapper->transform(dx, dy);
    } else if (tool->isCreationTool()) {
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
    if (shouldIgnoreInput() || event->button() & ~acceptedMouseButtons()) {
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
                   && (!selectedOptions.testFlag(AnnotationTool::TextOption) || wrapper->text().isEmpty())) {
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

QSGNode *AnnotationViewport::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (!m_document || width() <= 0 || height() <= 0) {
        delete oldNode;
        return nullptr;
    }

    const auto window = this->window();
    auto node = static_cast<AnnotationViewportNode *>(oldNode);
    if (!node) {
        node = new AnnotationViewportNode(window->createImageNode(), //
                                            window->createImageNode());
        node->baseImageNode()->setFiltering(QSGTexture::Linear);
        node->annotationsNode()->setFiltering(QSGTexture::Linear);
        // Setting the mipmap filter type also enables mipmaps.
        // Super useful for scaling down smoothly.
        node->baseImageNode()->setMipmapFiltering(QSGTexture::Linear);
        node->annotationsNode()->setMipmapFiltering(QSGTexture::Linear);
    }

    const auto canvasRect = m_document->canvasRect();
    const auto imageDpr = m_document->imageDpr();
    const auto windowDpr = window->effectiveDevicePixelRatio();
    const auto canvasView = canvasRect.intersected(m_viewportRect.translated(canvasRect.topLeft()));
    const auto imageView = G::rectScaled(canvasView.translated(-canvasRect.topLeft()), imageDpr);

    auto baseImageNode = node->baseImageNode();
    if (!baseImageNode->texture() || m_repaintBaseImage) {
        auto image = m_document->canvasBaseImage();
        QRectF sourceBounds = image.rect();
        auto sourceRect = imageView.intersected(sourceBounds).toRect();
        if (sourceRect != sourceBounds) {
            image = image.copy(sourceRect);
        }
        baseImageNode->setTexture(window->createTextureFromImage(image));
        m_repaintBaseImage = false;
    }

    auto annotationsNode = node->annotationsNode();
    if (!annotationsNode->texture() || m_repaintAnnotations) {
        auto image = m_document->annotationsImage();
        QRectF sourceBounds = image.rect();
        auto sourceRect = imageView.intersected(sourceBounds).toRect();
        if (sourceRect != sourceBounds) {
            image = image.copy(sourceRect);
        }
        annotationsNode->setTexture(window->createTextureFromImage(image));
        m_repaintAnnotations = false;
    }

    auto setupImageNode = [&](QSGImageNode *node) {
        QRectF targetRect{{0, 0}, node->texture()->textureSize().toSizeF().scaled(this->size(), Qt::KeepAspectRatio)};
        targetRect.moveTo(std::round((width() - targetRect.width()) / 2 * windowDpr) / windowDpr,
                          std::round((height() - targetRect.height()) / 2 * windowDpr) / windowDpr);
        if (!targetRect.isEmpty()) {
            node->setRect(targetRect);
        }
    };

    setupImageNode(baseImageNode);
    setupImageNode(annotationsNode);

    return node;
}

void AnnotationViewport::itemChange(ItemChange change, const ItemChangeData &value)
{
    if (change == ItemDevicePixelRatioHasChanged) {
        update();
    }
    QQuickItem::itemChange(change, value);
}


bool AnnotationViewport::shouldIgnoreInput() const
{
    return !isEnabled() || !m_document || m_document->tool()->isNoTool();
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
