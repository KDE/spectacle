/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "AnnotationDocument.h"
#include "EffectUtils.h"
#include "Geometry.h"

#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <memory>

using G = Geometry;

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

QSizeF AnnotationDocument::canvasSize() const
{
    return m_canvasRect.size();
}

QSizeF AnnotationDocument::imageSize() const
{
    return m_image.size();
}

qreal AnnotationDocument::imageDpr() const
{
    return m_image.devicePixelRatio();
}

void AnnotationDocument::setImage(const QImage &image)
{
    m_image = image;
    m_canvasRect = {{0, 0}, QSizeF(image.size()) / image.devicePixelRatio()};

    Q_EMIT canvasSizeChanged();
    Q_EMIT imageSizeChanged();
    Q_EMIT imageDprChanged();
    Q_EMIT repaintNeeded();
}

void AnnotationDocument::cropCanvas(const QRectF &cropRect)
{
    if (cropRect == m_canvasRect) {
        return;
    }

    auto translate = QTransform::fromTranslate(-cropRect.x(), -cropRect.y());
    auto intersectsRect = [cropRect](History::List::const_reference item) {
        return item && item->renderRect().intersects(cropRect);
    };
    History::Lists filteredLists = m_history.filteredLists(intersectsRect);
    auto &filteredUndo = filteredLists.undoList;
    auto &filteredRedo = filteredLists.redoList;
    for (auto it = filteredUndo.begin(); it != filteredUndo.end(); ++it) {
        Traits::transformTraits(translate, it->get()->traits());
    }
    for (auto it = filteredRedo.begin(); it != filteredRedo.end(); ++it) {
        Traits::transformTraits(translate, it->get()->traits());
    }
    m_history = {filteredUndo, filteredRedo};

    setImage(m_image.copy(G::rectScaled(cropRect, imageDpr()).toAlignedRect()));

    Q_EMIT canvasSizeChanged();
    Q_EMIT undoStackDepthChanged();
    Q_EMIT redoStackDepthChanged();
    Q_EMIT repaintNeeded();
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
    Q_EMIT repaintNeeded();
}

void AnnotationDocument::clear()
{
    clearAnnotations();
    setImage({});
}

void AnnotationDocument::paintBaseImage(QPainter *painter, const Viewport &viewport) const
{
    painter->save();
    auto imageRect = G::rectScaled(viewport.rect, imageDpr() / viewport.scale);
    // Enable smooth transform for fractional scales.
    painter->setRenderHint(QPainter::SmoothPixmapTransform, fmod(imageDpr() / viewport.scale, 1) != 0);
    if (viewport.scale == 1) {
        painter->drawImage({0, 0}, m_image, imageRect);
    } else {
        // More High quality scale down
        auto scaledImg = m_image.scaled(m_image.size() * viewport.scale, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        painter->drawImage({0, 0}, scaledImg, imageRect);
    }
    painter->restore();
}
void AnnotationDocument::paintAnnotations(QPainter *painter, const Viewport &viewport, std::optional<History::ConstSpan> span) const
{
    painter->save();
    painter->scale(viewport.scale, viewport.scale);
    painter->setRenderHints({QPainter::Antialiasing, QPainter::TextAntialiasing});
    const auto &undoList = m_history.undoList();
    if (!span) {
        span.emplace(undoList);
    }
    const auto begin = span->begin();
    for (auto it = begin; it != span->end(); ++it) {
        if (!m_history.itemVisible(*it)) {
            continue;
        }
        // Render the temporary item instead if this item is selected.
        auto selectedItem = m_selectedItemWrapper->selectedItem().lock();
        auto renderedItem = *it == selectedItem ? m_tempItem.get() : it->get();
        if (!renderedItem) {
            continue;
        }
        auto &geometry = std::get<Traits::Geometry::Opt>(renderedItem->traits());
        if (!geometry->visualRect.intersects(G::rectScaled(viewport.rect, 1 / viewport.scale))) {
            continue;
        }

        painter->save(); // Remember to call restore later.
        painter->translate(-viewport.rect.topLeft());
        painter->setPen(Qt::NoPen);
        painter->setBrush(Qt::NoBrush);

        auto &highlight = std::get<Traits::Highlight::Opt>(renderedItem->traits());
        painter->setCompositionMode(highlight ? highlight->compositionMode : QPainter::CompositionMode_SourceOver);

        // Draw the shadow if existent
        auto &shadow = std::get<Traits::Shadow::Opt>(renderedItem->traits());
        if (shadow && shadow->enabled) {
            QImage image = shapeShadow(renderedItem->traits());
            painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
            painter->drawImage(geometry->visualRect, image);
            painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
        }

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
                auto getImage = [this, begin, it] {
                    return renderToImage(std::span{begin, it});
                };
                const auto &rect = geometry->path.boundingRect();
                const auto &image = blur.image(getImage, rect, imageDpr());
                painter->drawImage(rect, image);
            } break;
            case Traits::Fill::Pixelate: {
                auto &pixelate = std::get<Fill::Pixelate>(fill);
                auto getImage = [this, begin, it] {
                    return renderToImage(std::span{begin, it});
                };
                const auto &rect = geometry->path.boundingRect();
                const auto &image = pixelate.image(getImage, rect, imageDpr());
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

        painter->restore();
    }
    painter->restore();
}

void AnnotationDocument::paint(QPainter *painter, const Viewport &viewport, std::optional<History::ConstSpan> span) const
{
    paintBaseImage(painter, viewport);
    paintAnnotations(painter, viewport, span);
}

QImage AnnotationDocument::renderToImage(const Viewport &viewport, std::optional<History::ConstSpan> span) const
{
    QImage img((viewport.rect.size() * imageDpr()).toSize(), QImage::Format_ARGB32_Premultiplied);
    img.setDevicePixelRatio(imageDpr());
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    // Makes pixelate and blur look better
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    paint(&p, viewport, span);
    p.end();

    return img;
}

QImage AnnotationDocument::renderToImage(std::optional<History::ConstSpan> span) const
{
    return renderToImage({m_canvasRect, 1}, span);
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
        emitRepaintNeededUnlessEmpty(result.item->renderRect());
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
    for (auto it = undoList.crbegin(); it != undoList.crend(); ++it) {
        auto &item = *it;
        if (m_history.itemVisible(item)) {
            auto &geometry = std::get<Traits::Geometry::Opt>(item->traits());
            if (geometry->mousePath.contains(rect.center())) {
                return item;
            }
        }
    }
    // If rect has no width or height
    if (rect.isNull()) {
        return nullptr;
    }
    // Forgiving if that failed so that you don't need to be perfect.
    for (auto it = undoList.crbegin(); it != undoList.crend(); ++it) {
        auto &item = *it;
        if (m_history.itemVisible(item)) {
            QPainterPath path(rect.topLeft());
            path.addEllipse(rect);
            auto &geometry = std::get<Traits::Geometry::Opt>(item->traits());
            if (geometry->mousePath.intersects(path)) {
                return item;
            }
        }
    }
    return nullptr;
}

void AnnotationDocument::undo()
{
    const auto undoCount = m_history.undoList().size();
    if (!undoCount) {
        return;
    }

    auto currentItem = m_history.currentItem();
    auto prevItem = undoCount > 1 ? m_history.undoList()[undoCount - 2] : nullptr;
    auto updateRect = currentItem->renderRect();
    if (prevItem) {
        updateRect |= prevItem->renderRect();
    }
    if (auto text = std::get<Traits::Text::Opt>(currentItem->traits())) {
        if (text->index() == Traits::Text::Number) {
            m_tool->setNumber(std::get<Traits::Text::Number>(text.value()));
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
    emitRepaintNeededUnlessEmpty(updateRect);
}

void AnnotationDocument::redo()
{
    if (m_history.redoList().empty()) {
        return;
    }

    auto currentItem = m_history.currentItem();
    auto nextItem = *m_history.redoList().crbegin();
    auto updateRect = nextItem->renderRect();
    if (currentItem) {
        updateRect |= currentItem->renderRect();
    }
    if (auto text = std::get<Traits::Text::Opt>(nextItem->traits())) {
        if (text->index() == Traits::Text::Number) {
            m_tool->setNumber(std::get<Traits::Text::Number>(text.value()) + 1);
        }
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
    emitRepaintNeededUnlessEmpty(updateRect);
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

    QRectF updateRect;
    // if the last item was not valid, discard it (for instance a rectangle with 0 size)
    if (!isCurrentItemValid()) {
        auto result = m_history.pop();
        if (result.item) {
            updateRect = result.item->renderRect();
        }
    }

    using enum AnnotationTool::Tool;
    using enum AnnotationTool::Option;
    HistoryItem temp;
    auto &geometry = std::get<Traits::Geometry::Opt>(temp.traits());
    geometry.emplace(QPainterPath{point}, QPainterPath{point}, QRectF{point, point});

    auto toolType = m_tool->type();
    auto toolOptions = m_tool->options();
    if (toolType == BlurTool) {
        auto &fill = std::get<Traits::Fill::Opt>(temp.traits());
        fill.emplace(Traits::ImageEffects::Blur{4});
    } else if (toolType == PixelateTool) {
        auto &fill = std::get<Traits::Fill::Opt>(temp.traits());
        fill.emplace(Traits::ImageEffects::Pixelate{4});
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
    updateRect |= newItem->renderRect();
    addItem(newItem);
    m_selectedItemWrapper->setSelectedItem(newItem);
    emitRepaintNeededUnlessEmpty(updateRect);
}

void AnnotationDocument::continueItem(const QPointF &point, ContinueOptions options)
{
    const auto &currentItem = m_history.currentItem();
    bool isSelected = m_selectedItemWrapper->selectedItem() == currentItem;
    const auto &item = isSelected ? m_tempItem : currentItem;
    if (!m_tool->isCreationTool() || !item || !std::get<Traits::Geometry::Opt>(item->traits())) {
        return;
    }

    auto renderRect = item->renderRect();
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
        renderRect |= m_selectedItemWrapper->reset();
        m_selectedItemWrapper->setSelectedItem(currentItem);
    }
    emitRepaintNeededUnlessEmpty(renderRect | item->renderRect());
}

void AnnotationDocument::finishItem()
{
    const auto &currentItem = m_history.currentItem();
    bool isSelected = m_selectedItemWrapper->selectedItem() == currentItem;
    const auto &item = isSelected ? m_tempItem : currentItem;
    if (!m_tool->isCreationTool() || !item || !std::get<Traits::Geometry::Opt>(item->traits())) {
        return;
    }

    Traits::initOptTuple(item->traits());
    if (isSelected) {
        *currentItem = *item;
        auto renderRect = m_selectedItemWrapper->reset();
        m_selectedItemWrapper->setSelectedItem(currentItem);
        Q_EMIT selectedItemWrapperChanged(); // re-evaluate qml bindings
        emitRepaintNeededUnlessEmpty(renderRect);
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
    addItem(newItem);
    deselectItem();
    emitRepaintNeededUnlessEmpty(selectedItem->renderRect());
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

void AnnotationDocument::emitRepaintNeededUnlessEmpty(const QRectF &area)
{
    if (!area.isEmpty()) {
        Q_EMIT repaintNeeded(area);
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
        || (historyItem && !std::get<Traits::Geometry::Opt>(historyItem->traits()))) {
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

        auto &text = std::get<Traits::Text::Opt>(temp->traits());
        m_options.setFlag(AnnotationTool::FontOption, text.has_value());
        m_options.setFlag(AnnotationTool::TextOption, //
                          text && text->index() == Traits::Text::String);
        m_options.setFlag(AnnotationTool::NumberOption, //
                          text && text->index() == Traits::Text::Number);

        m_options.setFlag(AnnotationTool::ShadowOption, //
                          std::get<Traits::Shadow::Opt>(temp->traits()).has_value());
    } else {
        m_document->emitRepaintNeededUnlessEmpty(reset());
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
    auto oldRenderRect = temp->renderRect();
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
    const auto &newRenderRect = temp->renderRect();
    Q_EMIT mousePathChanged();
    m_document->emitRepaintNeededUnlessEmpty(oldRenderRect | newRenderRect);
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

QRectF SelectedItemWrapper::reset()
{
    auto &temp = m_document->m_tempItem;
    if (!hasSelection() && m_options == AnnotationTool::NoOptions) {
        return {};
    }
    QRectF renderRect;
    if (temp) {
        auto selectedItem = m_selectedItem.lock();
        if (selectedItem) {
            renderRect = selectedItem->renderRect();
        }
        renderRect |= temp->renderRect();
    }
    temp.reset();
    m_selectedItem.reset();
    m_options = AnnotationTool::NoOptions;
    // Not emitting selectedItemWrapperChanged.
    // Use the return value to determine if that should be done when necessary.
    return renderRect;
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
    auto oldRect = temp->renderRect();
    stroke->pen.setWidthF(width);
    Traits::reInitTraits(temp->traits());
    const auto &newRect = temp->renderRect();
    Q_EMIT strokeWidthChanged();
    if (oldRect != newRect) {
        Q_EMIT mousePathChanged();
    }
    m_document->emitRepaintNeededUnlessEmpty(oldRect | newRect);
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
    m_document->emitRepaintNeededUnlessEmpty(temp->renderRect());
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
    m_document->emitRepaintNeededUnlessEmpty(temp->renderRect());
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
    auto oldRect = temp->renderRect();
    text->font = font;
    Traits::reInitTraits(temp->traits());
    const auto &newRect = temp->renderRect();
    Q_EMIT fontChanged();
    if (oldRect != newRect) {
        Q_EMIT mousePathChanged();
    }
    m_document->emitRepaintNeededUnlessEmpty(oldRect | newRect);
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
    m_document->emitRepaintNeededUnlessEmpty(temp->renderRect());
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
    auto oldRect = temp->renderRect();
    text.value().emplace<Traits::Text::Number>(number);
    Traits::reInitTraits(temp->traits());
    const auto &newRect = temp->renderRect();
    Q_EMIT numberChanged();
    if (oldRect != newRect) {
        Q_EMIT mousePathChanged();
    }
    m_document->emitRepaintNeededUnlessEmpty(oldRect | newRect);
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
    auto oldRect = temp->renderRect();
    text.value().emplace<Traits::Text::String>(string);
    Traits::reInitTraits(temp->traits());
    const auto &newRect = temp->renderRect();
    Q_EMIT textChanged();
    if (oldRect != newRect) {
        Q_EMIT mousePathChanged();
    }
    m_document->emitRepaintNeededUnlessEmpty(oldRect | newRect);
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
    auto oldRect = temp->renderRect();
    shadow->enabled = enabled;
    Traits::reInitTraits(temp->traits());
    Q_EMIT shadowChanged();
    m_document->emitRepaintNeededUnlessEmpty(oldRect | temp->renderRect());
}

QPainterPath SelectedItemWrapper::mousePath() const
{
    auto &temp = m_document->m_tempItem;
    if (!hasSelection()) {
        return {};
    }
    return Traits::mousePath(temp->traits());
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
