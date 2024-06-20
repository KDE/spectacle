/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "AnnotationDocument.h"
#include "EffectUtils.h"
#include "Geometry.h"
#include "DebugUtils.h"

#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <memory>

using G = Geometry;

QImage defaultImage(const QSize &size, qreal dpr)
{
    // RGBA is better for use with OpenCV
    QImage image(size, QImage::Format_RGBA8888_Premultiplied);
    image.setDevicePixelRatio(dpr);
    image.fill(Qt::transparent);
    return image;
}

inline QRectF deviceIndependentRect(const QImage &image)
{
    return {{0, 0}, image.deviceIndependentSize()};
}

AnnotationDocument::AnnotationDocument(QObject *parent)
    : QObject(parent)
    , m_tool(new AnnotationTool(this))
    , m_selectedItemWrapper(new SelectedItemWrapper(this))
{
}

AnnotationDocument::~AnnotationDocument()
{
}

AnnotationTool *AnnotationDocument::tool() const
{
    return m_tool;
}

SelectedItemWrapper *AnnotationDocument::selectedItemWrapper() const
{
    return m_selectedItemWrapper;
}

int AnnotationDocument::undoStackDepth() const
{
    return m_history.undoList().size();
}

int AnnotationDocument::redoStackDepth() const
{
    return m_history.redoList().size();
}

QRectF AnnotationDocument::canvasRect() const
{
    return m_canvasRect;
}

void AnnotationDocument::setCanvas(const QRectF &rect, qreal dpr)
{
    // Don't allow an invalid canvas rect or device pixel ratio.
    if (rect.isEmpty()) {
        Log::warning() << std::format("`{}`:\n\t`rect` is empty. This should not happen.",
                                      std::source_location::current().function_name());
        return;
    } else if (dpr <= 0) {
        Log::warning() << std::format("`{}`:\n\t`dpr` <= 0. This should not happen.",
                                      std::source_location::current().function_name());
        return;
    }
    const bool posChanged = m_canvasRect.topLeft() != rect.topLeft();
    const bool sizeChanged = m_canvasRect.size() != rect.size();
    const bool dprChanged = m_imageDpr != dpr;
    if (posChanged || sizeChanged) {
        m_canvasRect = rect;
        Q_EMIT canvasRectChanged();
    }
    if (dprChanged) {
        m_imageDpr = dpr;
        Q_EMIT imageDprChanged();
    }
    if (sizeChanged || dprChanged) {
        m_imageSize = (rect.size() * dpr).toSize();
        m_annotationsImage = defaultImage(m_imageSize, dpr);
        Q_EMIT imageSizeChanged();
    }
    // Reset cropped image
    if (!m_baseImage.isNull()) {
        const auto imageDIRect = deviceIndependentRect(m_baseImage);
        if (m_canvasRect.contains(imageDIRect)) {
            m_croppedBaseImage = {};
        } else {
            const auto imageRect = G::rectScaled(m_canvasRect.intersected(imageDIRect), m_imageDpr).toRect();
            m_croppedBaseImage = m_baseImage.copy(imageRect);
        }
    } else if (!m_croppedBaseImage.isNull()) {
        m_croppedBaseImage = {};
    }
    // Unconditionally repaint the whole canvas area
    setRepaintRegion();
}

void AnnotationDocument::resetCanvas()
{
    setCanvas(deviceIndependentRect(m_baseImage), m_baseImage.devicePixelRatio());
}

QSizeF AnnotationDocument::imageSize() const
{
    return m_imageSize;
}

qreal AnnotationDocument::imageDpr() const
{
    return m_imageDpr;
}

QImage AnnotationDocument::baseImage() const
{
    return m_baseImage;
}

QImage AnnotationDocument::canvasBaseImage() const
{
    if (m_baseImage.isNull() || m_croppedBaseImage.isNull()) {
        return m_baseImage;
    }
    return m_croppedBaseImage;
}

void AnnotationDocument::setBaseImage(const QImage &image)
{
    if (m_baseImage.cacheKey() == image.cacheKey()) {
        return;
    }
    m_baseImage = image;
    resetCanvas();
}

void AnnotationDocument::cropCanvas(const QRectF &cropRect)
{
    // Can't crop to nothing
    if (cropRect.isEmpty()) {
        return;
    }

    // In the UI, (0,0) for the crop rect will be the top left of the current canvas rect.
    // A crop can only make the canvas smaller.
    auto newCanvasRect = cropRect.translated(m_canvasRect.topLeft()).intersected(m_canvasRect);
    if (newCanvasRect == m_canvasRect) {
        return;
    }

    auto newItem = std::make_shared<HistoryItem>();
    QPainterPath path;
    path.addRect(newCanvasRect);
    std::get<Traits::Geometry::Opt>(newItem->traits()).emplace(path);
    std::get<Traits::Meta::Crop::Opt>(newItem->traits()).emplace();
    const auto &undoList = m_history.undoList();
    for (auto it = std::ranges::crbegin(undoList); it != std::ranges::crend(undoList); ++it) {
        const auto item = *it;
        if (!item) {
            continue;
        }
        if (std::get<Traits::Meta::Crop::Opt>(item->traits()).has_value()) {
            HistoryItem::setItemRelations(item, newItem);
            break;
        }
    }
    setCanvas(newCanvasRect, m_imageDpr);
    addItem(newItem);
}

void AnnotationDocument::clearAnnotations()
{
    auto result = m_history.clearLists();
    m_tool->resetType();
    m_tool->resetNumber();
    deselectItem();
    if (result.undoListChanged) {
        Q_EMIT undoStackDepthChanged();
    }
    if (result.redoListChanged) {
        Q_EMIT redoStackDepthChanged();
    }
    setRepaintRegion(RepaintType::Annotations);
}

void AnnotationDocument::clear()
{
    clearAnnotations();
    setBaseImage({});
}

void AnnotationDocument::paintImageView(QPainter *painter, const QImage &image, const QRectF &viewport) const
{
    if (!painter || image.isNull()) {
        return;
    }
    // Enable smooth transform for fractional scales.
    painter->setRenderHint(QPainter::SmoothPixmapTransform, fmod(imageDpr(), 1) != 0);
    if (viewport.isNull()) {
        painter->drawImage(QPointF{0, 0}, image);
    } else {
        painter->drawImage({0, 0}, image, G::rectScaled(viewport, imageDpr()));
    }
}

void AnnotationDocument::paintAnnotations(QPainter *painter, const QRegion &region, std::optional<History::SubRange> range) const
{
    if (!painter || region.isEmpty()) {
        return;
    }
    const auto &undoList = m_history.undoList();
    if (undoList.empty() || (range && range->empty())) {
        return;
    }
    if (!range) {
        range.emplace(undoList);
    }

    const auto begin = range->begin();
    const auto end = range->end();
    // Only highlighter needs the base image to be rendered underneath itself to function correctly.
    const bool hasHighlighter = std::any_of(begin, end, [this, &region](const HistoryItem::const_shared_ptr &item) {
        const auto &renderedItem = item == m_selectedItemWrapper->selectedItem() ? m_tempItem : item;
        if (!renderedItem) {
            return false;
        }
        auto &visual = std::get<Traits::Visual::Opt>(renderedItem->traits());
        if (!visual) {
            return false;
        }
        return std::get<Traits::Highlight::Opt>(renderedItem->traits()).has_value() //
            && m_history.itemVisible(item) && region.intersects(visual->rect.toAlignedRect());
    });
    if (hasHighlighter) {
        bool hasDifferentClip = false;
        QRegion oldRegion;
        if (painter->hasClipping()) {
            oldRegion = painter->clipRegion();
            hasDifferentClip = oldRegion != region;
            if (hasDifferentClip) {
                painter->setClipRegion(region);
            }
        }
        paintImageView(painter, m_baseImage);
        if (hasDifferentClip) {
            painter->setClipRegion(oldRegion);
        }
    }
    for (auto it = begin; it != end; ++it) {
        const auto item = *it;
        if (!m_history.itemVisible(item)) {
            continue;
        }
        // Render the temporary item instead if this item is selected.
        const auto &renderedItem = item == m_selectedItemWrapper->selectedItem() ? m_tempItem : item;
        if (!renderedItem) {
            continue;
        }
        auto &visual = std::get<Traits::Visual::Opt>(renderedItem->traits());
        if (!region.intersects(visual->rect.toAlignedRect())) {
            continue;
        }

        painter->setRenderHints({QPainter::Antialiasing, QPainter::TextAntialiasing});
        painter->setPen(Qt::NoPen);
        painter->setBrush(Qt::NoBrush);

        auto &highlight = std::get<Traits::Highlight::Opt>(renderedItem->traits());
        painter->setCompositionMode(highlight ? highlight->compositionMode : QPainter::CompositionMode_SourceOver);

        // Draw the shadow if existent
        auto &shadow = std::get<Traits::Shadow::Opt>(renderedItem->traits());
        if (shadow && shadow->enabled) {
            QImage image = shapeShadow(renderedItem->traits());
            painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
            painter->drawImage(visual->rect, image);
        }

        auto &geometry = std::get<Traits::Geometry::Opt>(renderedItem->traits());
        if (auto &fillOpt = std::get<Traits::Fill::Opt>(renderedItem->traits())) {
            using namespace Traits;
            auto &fill = fillOpt.value();
            switch (fill.index()) {
            case Fill::Brush:
                painter->setBrush(std::get<Fill::Brush>(fill));
                painter->drawPath(geometry->path);
                break;
            case Traits::Fill::Blur: {
                auto &blur = std::get<Fill::Blur>(fill);
                auto untilNow = History::SubRange{begin, it};
                auto getImage = [this, untilNow] {
                    return rangeImage(untilNow);
                };
                const auto &rect = geometry->path.boundingRect();
                const auto &image = blur.image(getImage, rect, imageDpr());
                painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
                painter->drawImage(rect, image);
            } break;
            case Traits::Fill::Pixelate: {
                auto &pixelate = std::get<Fill::Pixelate>(fill);
                auto untilNow = History::SubRange{begin, it};
                auto getImage = [this, untilNow] {
                    return rangeImage(untilNow);
                };
                const auto &rect = geometry->path.boundingRect();
                const auto &image = pixelate.image(getImage, rect, imageDpr());
                painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
                painter->drawImage(rect, image);
            } break;
            default:
                break;
            }
        }

        if (auto &stroke = std::get<Traits::Stroke::Opt>(renderedItem->traits())) {
            painter->setBrush(stroke->pen.brush());
            painter->drawPath(stroke->path);
        }

        if (auto &text = std::get<Traits::Text::Opt>(renderedItem->traits())) {
            painter->setBrush(Qt::NoBrush);
            painter->setPen(text->brush.color());
            painter->setFont(text->font);
            painter->drawText(geometry->path.boundingRect(), text->textFlags(), text->text());
        }
    }
}

QImage AnnotationDocument::annotationsImage()
{
    if (m_annotationsImage.isNull()) {
        return m_annotationsImage;
    }
    if (!m_repaintRegion.isEmpty()) {
        QPainter painter(&m_annotationsImage);
        // canvas rect top left should be (0,0) in annotations image
        painter.translate(-m_canvasRect.topLeft());
        // Set clip region to prevent over-painting shadows or semi-transparent annotations near the region.
        painter.setClipRegion(m_repaintRegion);
        // Clear mode is needed to actually clear the region.
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        // The painter is clipped to the region, so we can just use eraseRect.
        painter.eraseRect(m_repaintRegion.boundingRect());
        // Restore default composition mode.
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        paintAnnotations(&painter, m_repaintRegion);
        painter.end();
        m_repaintRegion = {};
    }
    return m_annotationsImage;
}

QImage AnnotationDocument::renderToImage()
{
    auto image = canvasBaseImage();
    QPainter painter(&image);
    paintImageView(&painter, annotationsImage());
    painter.end();
    return image;
}

QImage AnnotationDocument::rangeImage(History::SubRange range) const
{
    auto image = m_baseImage;
    QPainter p(&image);
    paintAnnotations(&p, deviceIndependentRect(m_baseImage).toAlignedRect(), range);
    p.end();
    return image;
}

bool AnnotationDocument::isCurrentItemValid() const
{
    return m_history.currentItem() && m_history.currentItem()->isValid();
}

HistoryItem::shared_ptr AnnotationDocument::popCurrentItem()
{
    auto result = m_history.pop();
    if (result.item) {
        if (result.item == m_selectedItemWrapper->selectedItem().lock()) {
            deselectItem();
        }
        Q_EMIT undoStackDepthChanged();
        setRepaintRegion(result.item->renderRect());
    }
    if (result.redoListChanged) {
        Q_EMIT redoStackDepthChanged();
    }
    return result.item;
}

HistoryItem::const_shared_ptr AnnotationDocument::itemAt(const QRectF &rect) const
{
    const auto &undoList = m_history.undoList();
    // Precisely the first time so that users can get exactly what they click.
    for (auto it = std::ranges::crbegin(undoList); it != std::ranges::crend(undoList); ++it) {
        const auto item = *it;
        if (m_history.itemVisible(item)) {
            auto &interactive = std::get<Traits::Interactive::Opt>(item->traits());
            if (interactive->path.contains(rect.center())) {
                return item;
            }
        }
    }
    // If rect has no width or height
    if (rect.isNull()) {
        return nullptr;
    }
    // Forgiving if that failed so that you don't need to be perfect.
    for (auto it = std::ranges::crbegin(undoList); it != std::ranges::crend(undoList); ++it) {
        const auto item = *it;
        if (m_history.itemVisible(item)) {
            QPainterPath path(rect.topLeft());
            path.addEllipse(rect);
            auto &interactive = std::get<Traits::Interactive::Opt>(item->traits());
            if (interactive->path.intersects(path)) {
                return item;
            }
        }
    }
    return nullptr;
}

void AnnotationDocument::undo()
{
    const auto &undoList = m_history.undoList();
    const auto undoCount = undoList.size();
    if (!undoCount) {
        return;
    }

    auto currentItem = m_history.currentItem();
    auto prevItem = undoCount > 1 ? undoList[undoCount - 2] : nullptr;
    setRepaintRegion(currentItem->renderRect());
    if (prevItem) {
        setRepaintRegion(prevItem->renderRect());
    }
    if (auto text = std::get<Traits::Text::Opt>(currentItem->traits())) {
        if (text->index() == Traits::Text::Number) {
            m_tool->setNumber(std::get<Traits::Text::Number>(text.value()));
        }
    }
    if (std::get<Traits::Meta::Crop::Opt>(currentItem->traits()).has_value()) {
        auto parent = currentItem->parent().lock();
        if (parent) {
            setCanvas(Traits::geometryPathBounds(parent->traits()), m_imageDpr);
        } else if (!m_baseImage.isNull()) {
            resetCanvas();
        }
    }
    if (currentItem == m_selectedItemWrapper->selectedItem().lock()) {
        if (prevItem && currentItem->hasParent() && (prevItem == currentItem->parent())) {
            m_selectedItemWrapper->setSelectedItem(prevItem);
        } else {
            deselectItem();
        }
    }
    m_history.undo();

    Q_EMIT undoStackDepthChanged();
    Q_EMIT redoStackDepthChanged();
}

void AnnotationDocument::redo()
{
    const auto &redoList = m_history.redoList();
    if (redoList.empty()) {
        return;
    }

    auto currentItem = m_history.currentItem();
    auto nextItem = *std::ranges::crbegin(redoList);
    setRepaintRegion(nextItem->renderRect());
    if (currentItem) {
        setRepaintRegion(currentItem->renderRect());
    }
    if (auto text = std::get<Traits::Text::Opt>(nextItem->traits())) {
        if (text->index() == Traits::Text::Number) {
            m_tool->setNumber(std::get<Traits::Text::Number>(text.value()) + 1);
        }
    }
    if (std::get<Traits::Meta::Crop::Opt>(nextItem->traits()).has_value()) {
        setCanvas(Traits::geometryPathBounds(nextItem->traits()), m_imageDpr);
    }
    if (currentItem && currentItem == m_selectedItemWrapper->selectedItem()) {
        if (nextItem == currentItem->child()) {
            m_selectedItemWrapper->setSelectedItem(nextItem);
        } else {
            deselectItem();
        }
    }
    m_history.redo();

    Q_EMIT undoStackDepthChanged();
    Q_EMIT redoStackDepthChanged();
}

bool isAnyOfToolType(AnnotationTool::Tool type, auto... args)
{
    return ((type == args) || ...);
}

void AnnotationDocument::beginItem(const QPointF &point)
{
    if (!m_tool->isCreationTool()) {
        return;
    }

    // if the last item was not valid, discard it (for instance a rectangle with 0 size)
    if (!isCurrentItemValid()) {
        auto result = m_history.pop();
        if (result.item) {
            setRepaintRegion(result.item->renderRect());
        }
    }

    using enum AnnotationTool::Tool;
    using enum AnnotationTool::Option;
    HistoryItem temp;
    auto &geometry = std::get<Traits::Geometry::Opt>(temp.traits());
    geometry.emplace(QPainterPath{point});
    auto &interactive = std::get<Traits::Interactive::Opt>(temp.traits());
    interactive.emplace(QPainterPath{point});
    auto &visual = std::get<Traits::Visual::Opt>(temp.traits());
    visual.emplace(QRectF{point, point});

    auto toolType = m_tool->type();
    auto toolOptions = m_tool->options();
    if (toolType == BlurTool) {
        auto &fill = std::get<Traits::Fill::Opt>(temp.traits());
        fill.emplace(Traits::ImageEffects::Blur{m_tool->strength()});
    } else if (toolType == PixelateTool) {
        auto &fill = std::get<Traits::Fill::Opt>(temp.traits());
        fill.emplace(Traits::ImageEffects::Pixelate{m_tool->strength()});
    } else if (toolOptions.testFlag(FillOption)) {
        auto &fill = std::get<Traits::Fill::Opt>(temp.traits());
        fill.emplace(m_tool->fillColor());
    }

    if (toolOptions.testFlag(StrokeOption)) {
        auto &stroke = std::get<Traits::Stroke::Opt>(temp.traits());
        auto pen = Traits::Stroke::defaultPen();
        pen.setBrush(m_tool->strokeColor());
        pen.setWidthF(m_tool->strokeWidth());
        stroke.emplace(pen);
    }

    if (toolOptions.testFlag(ShadowOption)) {
        auto &shadow = std::get<Traits::Shadow::Opt>(temp.traits());
        shadow.emplace(m_tool->hasShadow());
    }

    if (isAnyOfToolType(toolType, FreehandTool, HighlighterTool)) {
        geometry->path = Traits::minPath(geometry->path);
    }
    if (toolType == HighlighterTool) {
        std::get<Traits::Highlight::Opt>(temp.traits()).emplace();
    } else if (toolType == ArrowTool) {
        std::get<Traits::Arrow::Opt>(temp.traits()).emplace();
    } else if (toolType == NumberTool) {
        std::get<Traits::Text::Opt>(temp.traits()).emplace(m_tool->number(), m_tool->fontColor(), m_tool->font());
        m_tool->setNumber(m_tool->number() + 1);
    } else if (toolType == TextTool) {
        std::get<Traits::Text::Opt>(temp.traits()).emplace(QString{}, m_tool->fontColor(), m_tool->font());
    }

    Traits::initOptTuple(temp.traits());

    auto newItem = std::make_shared<HistoryItem>(std::move(temp));
    setRepaintRegion(newItem->renderRect());
    addItem(newItem);
    m_selectedItemWrapper->setSelectedItem(newItem);
}

void AnnotationDocument::continueItem(const QPointF &point, ContinueOptions options)
{
    const auto &currentItem = m_history.currentItem();
    bool isSelected = m_selectedItemWrapper->selectedItem() == currentItem;
    const auto &item = isSelected ? m_tempItem : currentItem;
    if (!m_tool->isCreationTool() || !item || !Traits::canBeVisible(item->traits())) {
        return;
    }

    setRepaintRegion(item->renderRect());
    auto &geometry = std::get<Traits::Geometry::Opt>(item->traits());
    auto &path = geometry->path;
    switch (m_tool->type()) {
    case AnnotationTool::FreehandTool:
    case AnnotationTool::HighlighterTool: {
        auto prev = path.currentPosition();
        // smooth path as we go.
        path.quadTo(prev, (prev + point) / 2);
    } break;
    case AnnotationTool::LineTool:
    case AnnotationTool::ArrowTool: {
        auto count = path.elementCount();
        auto lastElement = path.elementAt(count - 1);
        QPointF endPoint = point;
        if (options & ContinueOption::SnapAngle) {
            const auto &prevElement = count > 1 ? path.elementAt(count - 2) : lastElement;
            QPointF posDiff = point - prevElement;
            if (qAbs(posDiff.x()) / 1.5 > qAbs(posDiff.y())) {
                // horizontal
                endPoint.setY(prevElement.y);
            } else if (qAbs(posDiff.x()) < qAbs(posDiff.y()) / 1.5) {
                // vertical
                endPoint.setX(prevElement.x);
            } else {
                // diagonal when 1/3 in between horizontal and vertical
                auto xSign = std::copysign(1.0, posDiff.x());
                auto ySign = std::copysign(1.0, posDiff.y());
                qreal max = qMax(qAbs(posDiff.x()), qAbs(posDiff.y()));
                endPoint = prevElement + QPointF(max * xSign, max * ySign);
            }
        }
        if (count > 1 && !lastElement.isMoveTo()) {
            path.setElementPositionAt(count - 1, endPoint.x(), endPoint.y());
        } else {
            path.lineTo(endPoint);
        }
    } break;
    case AnnotationTool::RectangleTool:
    case AnnotationTool::EllipseTool:
    case AnnotationTool::BlurTool:
    case AnnotationTool::PixelateTool: {
        const auto count = path.elementCount();
        // We always make the real start point the last point so we can easily keep it without
        // needing to keep a separate point or rectangle. Qt automatically moves the first MoveTo
        // element if one exists, so we can't just keep it at the start.
        auto start = path.currentPosition();
        // Can have a negative size with bottom right being visually top left.
        QRectF rect(start, point);
        if (options & ContinueOption::SnapAngle) {
            auto wSign = std::copysign(1.0, rect.width());
            auto hSign = std::copysign(1.0, rect.height());
            qreal max = qMax(qAbs(rect.width()), qAbs(rect.height()));
            rect.setSize({max * wSign, max * hSign});
        }

        if (options & ContinueOption::CenterResize) {
            if (count > 1) {
                auto oldBounds = path.boundingRect();
                rect.moveCenter(oldBounds.center());
            } else {
                rect.moveCenter(start);
            }
        }
        path.clear();
        if (m_tool->type() == AnnotationTool::EllipseTool) {
            path.addEllipse(rect);
        } else {
            path.addRect(rect);
        }
        // the top left is now the real start point
        path.moveTo(rect.topLeft());
    } break;
    case AnnotationTool::TextTool: {
        const auto count = path.elementCount();
        auto rect = path.boundingRect();
        if (count == 1) {
            // BUG: boundingRect won't have the correct position if the only element is a MoveTo.
            // Fixed in https://codereview.qt-project.org/c/qt/qtbase/+/534966.
            rect.moveTo(path.elementAt(0));
        }
        path.translate(point - QPointF{rect.x(), rect.center().y()});
    } break;
    case AnnotationTool::NumberTool: {
        const auto count = path.elementCount();
        auto rect = path.boundingRect();
        if (count == 1) {
            // BUG: boundingRect won't have the correct position if the only element is a MoveTo.
            // Fixed in https://codereview.qt-project.org/c/qt/qtbase/+/534966.
            rect.moveTo(path.elementAt(0));
        }
        path.translate(point - rect.center());
    } break;
    default:
        return;
    }

    Traits::clearForInit(item->traits());
    Traits::fastInitOptTuple(item->traits());

    if (isSelected) {
        *currentItem = *item;
        m_selectedItemWrapper->reset();
        m_selectedItemWrapper->setSelectedItem(currentItem);
    }
    setRepaintRegion(item->renderRect());
}

void AnnotationDocument::finishItem()
{
    const auto &currentItem = m_history.currentItem();
    bool isSelected = m_selectedItemWrapper->selectedItem() == currentItem;
    const auto &item = isSelected ? m_tempItem : currentItem;
    if (!m_tool->isCreationTool() || !item || !Traits::canBeVisible(item->traits())) {
        return;
    }

    Traits::initOptTuple(item->traits());
    if (isSelected) {
        *currentItem = *item;
        m_selectedItemWrapper->reset();
        m_selectedItemWrapper->setSelectedItem(currentItem);
        Q_EMIT selectedItemWrapperChanged(); // re-evaluate qml bindings
    }
}

void AnnotationDocument::selectItem(const QRectF &rect)
{
    m_selectedItemWrapper->setSelectedItem(itemAt(rect));
}

void AnnotationDocument::deselectItem()
{
    m_selectedItemWrapper->setSelectedItem(nullptr);
}

void AnnotationDocument::deleteSelectedItem()
{
    auto selectedItem = m_selectedItemWrapper->selectedItem().lock();
    if (!selectedItem) {
        return;
    }

    auto newItem = std::make_shared<HistoryItem>();
    HistoryItem::setItemRelations(selectedItem, newItem);
    std::get<Traits::Meta::Delete::Opt>(newItem->traits()).emplace();
    addItem(newItem);
    deselectItem();
    setRepaintRegion(selectedItem->renderRect());
}

void AnnotationDocument::addItem(const HistoryItem::shared_ptr &item)
{
    auto result = m_history.push(item);
    if (result.undoListChanged) {
        Q_EMIT undoStackDepthChanged();
    }
    if (result.redoListChanged) {
        Q_EMIT redoStackDepthChanged();
    }
}

void AnnotationDocument::setRepaintRegion(const QRectF &rect, RepaintTypes types)
{
    if (rect.isNull() || !m_canvasRect.intersects(rect)) {
        // No point in trying to transform or add to the region if not in the canvas rect.
        return;
    }
    // HACK: workaround not always repainting everywhere it should with fractional scaling.
    auto biggerRect = rect.normalized().adjusted(-1, -1, 0, 0).toAlignedRect();
    /* QRegion has a QRect overload for operator+=, but not for operator|=.
     *
     * `region += rect` is not the same as `region = region.united(rect)`. It will try to add the
     * rect directly instead of making a copy of itself with the rect added.
     *
     * QRegion only works with ints, so we need to convert the rect to image coordinates or ensure
     * The region contains a bit more than the rect with toAlignedRect.
     * We normalize the rect because operator+= will no-op if `rect.isEmpty()`.
     * `QRectF::isEmpty()` is true when the size is 0 or negative.
     */
    if (!m_canvasRect.intersects(biggerRect)) {
        // No point in trying to transform or add to the region if true.
        return;
    }
    const bool emitRepaintNeeded = m_repaintRegion.isEmpty() || m_lastRepaintTypes != types;
    m_repaintRegion += biggerRect;
    m_lastRepaintTypes = types;
    if (emitRepaintNeeded) {
        Q_EMIT repaintNeeded(m_lastRepaintTypes);
    }
}

void AnnotationDocument::setRepaintRegion(RepaintTypes types)
{
    const bool emitRepaintNeeded = m_repaintRegion.isEmpty() || m_lastRepaintTypes != types;
    m_repaintRegion = m_canvasRect.toAlignedRect();
    m_lastRepaintTypes = types;
    if (emitRepaintNeeded) {
        Q_EMIT repaintNeeded(m_lastRepaintTypes);
    }
}

//////////////////////////

SelectedItemWrapper::SelectedItemWrapper(AnnotationDocument *document)
    : QObject(document)
    , m_document(document)
{
}

SelectedItemWrapper::~SelectedItemWrapper()
{
}

HistoryItem::const_weak_ptr SelectedItemWrapper::selectedItem() const
{
    return m_selectedItem;
}

void SelectedItemWrapper::setSelectedItem(const HistoryItem::const_shared_ptr &historyItem)
{
    if (m_selectedItem == historyItem //
        || (historyItem && !Traits::canBeVisible(historyItem->traits()))) {
        return;
    }

    m_selectedItem = historyItem;
    if (historyItem) {
        auto &temp = m_document->m_tempItem;
        temp = std::make_shared<HistoryItem>(*historyItem);
        m_options.setFlag(AnnotationTool::StrokeOption, //
                          std::get<Traits::Stroke::Opt>(temp->traits()).has_value());

        auto &fill = std::get<Traits::Fill::Opt>(temp->traits());
        m_options.setFlag(AnnotationTool::FillOption, //
                          fill.has_value() && fill->index() == Traits::Fill::Brush);
        m_options.setFlag(AnnotationTool::StrengthOption, //
                          fill.has_value()
                              && (fill->index() == Traits::Fill::Blur //
                                  || fill->index() == Traits::Fill::Pixelate));

        auto &text = std::get<Traits::Text::Opt>(temp->traits());
        m_options.setFlag(AnnotationTool::FontOption, text.has_value());
        m_options.setFlag(AnnotationTool::TextOption, //
                          text && text->index() == Traits::Text::String);
        m_options.setFlag(AnnotationTool::NumberOption, //
                          text && text->index() == Traits::Text::Number);

        m_options.setFlag(AnnotationTool::ShadowOption, //
                          std::get<Traits::Shadow::Opt>(temp->traits()).has_value());
    } else {
        reset();
    }
    // all bindings using the selectedItem property should be re-evalulated when emitted
    Q_EMIT m_document->selectedItemWrapperChanged();
}

void SelectedItemWrapper::transform(qreal dx, qreal dy, Qt::Edges edges)
{
    auto selectedItem = m_selectedItem.lock();
    auto &temp = m_document->m_tempItem;
    if (!selectedItem || !temp || (qFuzzyIsNull(dx) && qFuzzyIsNull(dy))) {
        return;
    }
    m_document->setRepaintRegion(temp->renderRect());
    auto &oldPath = std::get<Traits::Geometry::Opt>(selectedItem->traits())->path;
    auto &path = std::get<Traits::Geometry::Opt>(temp->traits())->path;
    if (edges.toInt() == 0 //
        || edges.testFlags({Qt::TopEdge, Qt::LeftEdge, Qt::RightEdge, Qt::BottomEdge})) {
        const auto pathDelta = path.boundingRect().topLeft() - oldPath.boundingRect().topLeft();
        QTransform transform;
        transform.translate(dx - pathDelta.x(), dy - pathDelta.y());
        // This is less expensive since we don't regenerate stroke or mousePath when translating.
        Traits::transformTraits(transform, temp->traits());
    } else {
        const auto oldRect = oldPath.boundingRect();
        const auto leftEdge = edges.testFlag(Qt::LeftEdge);
        const auto topEdge = edges.testFlag(Qt::TopEdge);
        const auto newRect = oldRect.adjusted(leftEdge ? dx : 0, //
                                              topEdge ? dy : 0,
                                              edges.testFlag(Qt::RightEdge) ? dx : 0,
                                              edges.testFlag(Qt::BottomEdge) ? dy : 0);
        auto scale = Traits::scaleForSize(oldRect.size(), newRect.size());
        auto translation = Traits::unTranslateScale(scale.sx, scale.sy, oldRect.topLeft());
        translation.dx += leftEdge || oldRect.width() == 0 ? dx : 0;
        translation.dy += topEdge || oldRect.height() == 0 ? dy : 0;
        // Translate before scale to avoid scaling translation.
        auto transform = QTransform::fromTranslate(translation.dx, translation.dy);
        transform.scale(scale.sx, scale.sy);
        path = transform.map(oldPath);
        Traits::reInitTraits(temp->traits());
    }
    m_document->setRepaintRegion(temp->renderRect());
    Q_EMIT mousePathChanged();
}

bool SelectedItemWrapper::commitChanges()
{
    auto selectedItem = m_selectedItem.lock();
    auto &temp = m_document->m_tempItem;
    if (!selectedItem || !temp || !temp->isValid() //
        || temp->traits() == selectedItem->traits()) {
        return false;
    }

    if (!selectedItem->isValid() && selectedItem == m_document->m_history.currentItem()) {
        auto result = m_document->m_history.pop();
        if (result.redoListChanged) {
            Q_EMIT m_document->redoStackDepthChanged();
        }
    } else {
        HistoryItem::setItemRelations(selectedItem, temp);
    }
    m_document->addItem(temp);
    setSelectedItem(temp);
    return true;
}

bool SelectedItemWrapper::reset()
{
    auto &temp = m_document->m_tempItem;
    if (!hasSelection() && m_options == AnnotationTool::NoOptions) {
        return {};
    }
    bool selectionChanged = false;
    auto selectedItem = m_selectedItem.lock();
    if (selectedItem) {
        selectionChanged = true;
        m_document->setRepaintRegion(selectedItem->renderRect());
    }
    if (temp) {
        m_document->setRepaintRegion(temp->renderRect());
    }
    temp.reset();
    m_selectedItem.reset();
    m_options = AnnotationTool::NoOptions;
    // Not emitting selectedItemWrapperChanged.
    // Use the return value to determine if that should be done when necessary.
    return selectionChanged;
}

bool SelectedItemWrapper::hasSelection() const
{
    return !m_selectedItem.expired() && m_document->m_tempItem;
}

AnnotationTool::Options SelectedItemWrapper::options() const
{
    return m_options;
}

int SelectedItemWrapper::strokeWidth() const
{
    auto &temp = m_document->m_tempItem;
    if (!m_options.testFlag(AnnotationTool::StrokeOption) || !temp) {
        return 0;
    }
    auto &stroke = std::get<Traits::Stroke::Opt>(temp->traits());
    return stroke->pen.widthF();
}

void SelectedItemWrapper::setStrokeWidth(int width)
{
    auto &temp = m_document->m_tempItem;
    if (!m_options.testFlag(AnnotationTool::StrokeOption) || !temp) {
        return;
    }
    auto &stroke = std::get<Traits::Stroke::Opt>(temp->traits());
    if (stroke->pen.widthF() == width) {
        return;
    }
    m_document->setRepaintRegion(temp->renderRect());
    stroke->pen.setWidthF(width);
    Traits::reInitTraits(temp->traits());
    m_document->setRepaintRegion(temp->renderRect());
    Q_EMIT strokeWidthChanged();
    Q_EMIT mousePathChanged();
}

QColor SelectedItemWrapper::strokeColor() const
{
    auto &temp = m_document->m_tempItem;
    if (!m_options.testFlag(AnnotationTool::StrokeOption) || !temp) {
        return {};
    }
    auto &stroke = std::get<Traits::Stroke::Opt>(temp->traits());
    return stroke->pen.color();
}

void SelectedItemWrapper::setStrokeColor(const QColor &color)
{
    auto &temp = m_document->m_tempItem;
    if (!m_options.testFlag(AnnotationTool::StrokeOption) || !temp) {
        return;
    }
    auto &stroke = std::get<Traits::Stroke::Opt>(temp->traits());
    if (stroke->pen.color() == color) {
        return;
    }
    stroke->pen.setColor(color);
    Q_EMIT strokeColorChanged();
    m_document->setRepaintRegion(temp->renderRect());
}

QColor SelectedItemWrapper::fillColor() const
{
    auto &temp = m_document->m_tempItem;
    if (!m_options.testFlag(AnnotationTool::FillOption) || !temp) {
        return {};
    }
    auto &fill = std::get<Traits::Fill::Opt>(temp->traits()).value();
    auto &brush = std::get<Traits::Fill::Brush>(fill);
    return brush.color();
}

void SelectedItemWrapper::setFillColor(const QColor &color)
{
    auto &temp = m_document->m_tempItem;
    if (!m_options.testFlag(AnnotationTool::FillOption) || !temp) {
        return;
    }
    auto &fill = std::get<Traits::Fill::Opt>(temp->traits()).value();
    auto &brush = std::get<Traits::Fill::Brush>(fill);
    if (brush.color() == color) {
        return;
    }
    brush = color;
    Q_EMIT fillColorChanged();
    m_document->setRepaintRegion(temp->renderRect());
}

qreal SelectedItemWrapper::strength() const
{
    auto &temp = m_document->m_tempItem;
    if (!m_options.testFlag(AnnotationTool::StrengthOption) || !temp) {
        return {};
    }
    auto &fill = std::get<Traits::Fill::Opt>(temp->traits()).value();
    if (auto blur = std::get_if<Traits::Fill::Blur>(&fill)) {
        return blur->strength();
    } else if (auto pixelate = std::get_if<Traits::Fill::Pixelate>(&fill)) {
        return pixelate->strength();
    }
    return 0;
}

void SelectedItemWrapper::setStrength(qreal strength)
{
    auto &temp = m_document->m_tempItem;
    if (!m_options.testFlag(AnnotationTool::StrengthOption) || !temp) {
        return;
    }
    auto &fill = std::get<Traits::Fill::Opt>(temp->traits()).value();
    if (auto blur = std::get_if<Traits::Fill::Blur>(&fill); blur && blur->strength() != strength) {
        blur->setStrength(strength);
        Q_EMIT strengthChanged();
        m_document->setRepaintRegion(temp->renderRect());
    } else if (auto pixelate = std::get_if<Traits::Fill::Pixelate>(&fill); pixelate && pixelate->strength() != strength) {
        pixelate->setStrength(strength);
        Q_EMIT strengthChanged();
        m_document->setRepaintRegion(temp->renderRect());
    }
}

QFont SelectedItemWrapper::font() const
{
    auto &temp = m_document->m_tempItem;
    if (!m_options.testFlag(AnnotationTool::FontOption) || !temp) {
        return {};
    }
    auto &text = std::get<Traits::Text::Opt>(temp->traits());
    return text->font;
}

void SelectedItemWrapper::setFont(const QFont &font)
{
    auto &temp = m_document->m_tempItem;
    if (!m_options.testFlag(AnnotationTool::FontOption) || !temp) {
        return;
    }
    auto &text = std::get<Traits::Text::Opt>(temp->traits());
    if (text->font == font) {
        return;
    }
    m_document->setRepaintRegion(temp->renderRect());
    text->font = font;
    Traits::reInitTraits(temp->traits());
    m_document->setRepaintRegion(temp->renderRect());
    Q_EMIT fontChanged();
    Q_EMIT mousePathChanged();
}

QColor SelectedItemWrapper::fontColor() const
{
    auto &temp = m_document->m_tempItem;
    if (!m_options.testFlag(AnnotationTool::FontOption) || !temp) {
        return {};
    }
    auto &text = std::get<Traits::Text::Opt>(temp->traits());
    return text->brush.color();
}

void SelectedItemWrapper::setFontColor(const QColor &color)
{
    auto &temp = m_document->m_tempItem;
    if (!m_options.testFlag(AnnotationTool::FontOption) || !temp) {
        return;
    }
    auto &text = std::get<Traits::Text::Opt>(temp->traits());
    if (text->brush.color() == color) {
        return;
    }
    text->brush = color;
    Q_EMIT fontColorChanged();
    m_document->setRepaintRegion(temp->renderRect());
}

int SelectedItemWrapper::number() const
{
    auto &temp = m_document->m_tempItem;
    if (!m_options.testFlag(AnnotationTool::NumberOption) || !temp) {
        return 0;
    }
    auto &text = std::get<Traits::Text::Opt>(temp->traits());
    const auto *number = std::get_if<Traits::Text::Number>(&text.value());
    return number ? *number : 0;
}

void SelectedItemWrapper::setNumber(int number)
{
    auto &temp = m_document->m_tempItem;
    if (!m_options.testFlag(AnnotationTool::NumberOption) || !temp) {
        return;
    }
    auto &text = std::get<Traits::Text::Opt>(temp->traits());
    const auto *oldNumber = std::get_if<Traits::Text::Number>(&text.value());
    if (!oldNumber || *oldNumber == number) {
        return;
    }
    m_document->setRepaintRegion(temp->renderRect());
    text.value().emplace<Traits::Text::Number>(number);
    Traits::reInitTraits(temp->traits());
    m_document->setRepaintRegion(temp->renderRect());
    Q_EMIT numberChanged();
    Q_EMIT mousePathChanged();
}

QString SelectedItemWrapper::text() const
{
    auto &temp = m_document->m_tempItem;
    if (!m_options.testFlag(AnnotationTool::TextOption) || !temp) {
        return {};
    }
    auto &text = std::get<Traits::Text::Opt>(temp->traits());
    const auto *string = std::get_if<Traits::Text::String>(&text.value());
    return string ? *string : QString{};
}

void SelectedItemWrapper::setText(const QString &string)
{
    auto &temp = m_document->m_tempItem;
    if (!m_options.testFlag(AnnotationTool::TextOption) || !temp) {
        return;
    }
    auto &text = std::get<Traits::Text::Opt>(temp->traits());
    const auto *oldString = std::get_if<Traits::Text::String>(&text.value());
    if (!oldString || *oldString == string) {
        return;
    }
    m_document->setRepaintRegion(temp->renderRect());
    text.value().emplace<Traits::Text::String>(string);
    Traits::reInitTraits(temp->traits());
    m_document->setRepaintRegion(temp->renderRect());
    Q_EMIT textChanged();
    Q_EMIT mousePathChanged();
}

bool SelectedItemWrapper::hasShadow() const
{
    auto &temp = m_document->m_tempItem;
    if (!m_options.testFlag(AnnotationTool::ShadowOption) || !temp) {
        return false;
    }
    auto &shadow = std::get<Traits::Shadow::Opt>(temp->traits());
    return shadow ? shadow->enabled : false;
}

void SelectedItemWrapper::setShadow(bool enabled)
{
    auto &temp = m_document->m_tempItem;
    if (!m_options.testFlag(AnnotationTool::ShadowOption) || !temp) {
        return;
    }
    auto &shadow = std::get<Traits::Shadow::Opt>(temp->traits());
    if (shadow->enabled == enabled) {
        return;
    }
    m_document->setRepaintRegion(temp->renderRect());
    shadow->enabled = enabled;
    Traits::reInitTraits(temp->traits());
    m_document->setRepaintRegion(temp->renderRect());
    Q_EMIT shadowChanged();
}

QPainterPath SelectedItemWrapper::mousePath() const
{
    auto &temp = m_document->m_tempItem;
    if (!hasSelection()) {
        return {};
    }
    return Traits::interactivePath(temp->traits());
}

QDebug operator<<(QDebug debug, const SelectedItemWrapper *wrapper)
{
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << "SelectedItemWrapper(";
    if (!wrapper) {
        return debug << "0x0)";
    }
    debug << (const void *)wrapper;
    debug << ",\n  selectedItem=" << wrapper->selectedItem().lock().get();
    debug << ')';
    return debug;
}

#include <moc_AnnotationDocument.cpp>
