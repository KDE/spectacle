/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "AnnotationDocument.h"
#include "EditAction.h"
#include "EffectUtils.h"
#include "settings.h"

#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <cmath>
#include <memory>

static AnnotationTool::Options optionsForType(AnnotationDocument::EditActionType type)
{
    switch (type) {
    case AnnotationDocument::Highlight:
        return AnnotationTool::Stroke;
    case AnnotationDocument::FreeHand:
    case AnnotationDocument::Line:
    case AnnotationDocument::Arrow:
        return {AnnotationTool::Stroke, AnnotationTool::Shadow};
    case AnnotationDocument::Rectangle:
    case AnnotationDocument::Ellipse:
        return {AnnotationTool::Stroke, AnnotationTool::Shadow, AnnotationTool::Fill};
    case AnnotationDocument::Text:
        return {AnnotationTool::Font, AnnotationTool::Shadow};
    case AnnotationDocument::Number:
        return {AnnotationTool::Fill, AnnotationTool::Shadow, AnnotationTool::Font};
    default:
        return AnnotationTool::NoOptions;
    }
}

static int defaultStrokeWidthForType(AnnotationDocument::EditActionType type)
{
    switch (type) {
    case AnnotationDocument::FreeHand:
    case AnnotationDocument::Line:
    case AnnotationDocument::Arrow:
        return 4;
    case AnnotationDocument::Highlight:
        return 20;
    default: return 0;
    }
}

static QColor defaultStrokeColorForType(AnnotationDocument::EditActionType type)
{
    switch (type) {
    case AnnotationDocument::FreeHand:
    case AnnotationDocument::Line:
    case AnnotationDocument::Arrow:
        return Qt::red;
    case AnnotationDocument::Highlight:
        return Qt::yellow;
    case AnnotationDocument::Rectangle:
    case AnnotationDocument::Ellipse:
        return Qt::black;
    default: return Qt::transparent;
    }
}

static QColor defaultFillColorForType(AnnotationDocument::EditActionType type)
{
    switch (type) {
    case AnnotationDocument::Rectangle:
    case AnnotationDocument::Ellipse:
    case AnnotationDocument::Number:
        return Qt::red;
    default: return Qt::transparent;
    }
}

static QColor defaultFontColorForType(AnnotationDocument::EditActionType type)
{
    switch (type) {
    case AnnotationDocument::Text:
    case AnnotationDocument::Number:
        return Qt::black;
    default: return Qt::transparent;
    }
}

AnnotationTool::AnnotationTool(AnnotationDocument *document)
    : QObject(document)
{
}

AnnotationTool::~AnnotationTool()
{
}

AnnotationDocument::EditActionType AnnotationTool::type() const
{
    return m_type;
}

void AnnotationTool::setType(AnnotationDocument::EditActionType type)
{
    if (m_type == type) {
        return;
    }

    auto oldType = m_type;
    m_type = type;
    Q_EMIT typeChanged();

    auto newOptions = optionsForType(type);
    if (m_options != newOptions) {
        m_options = newOptions;
        Q_EMIT optionsChanged();
    }

    const auto &oldStrokeWidth = strokeWidthForType(oldType);
    const auto &newStrokeWidth = strokeWidthForType(type);
    if (oldStrokeWidth != newStrokeWidth) {
        Q_EMIT strokeWidthChanged(newStrokeWidth);
    }

    const auto &oldStrokeColor = strokeColorForType(oldType);
    const auto &newStrokeColor = strokeColorForType(type);
    if (oldStrokeColor != newStrokeColor) {
        Q_EMIT strokeColorChanged(newStrokeColor);
    }

    const auto &oldFillColor = fillColorForType(oldType);
    const auto &newFillColor = fillColorForType(type);
    if (oldFillColor != newFillColor) {
        Q_EMIT fillColorChanged(newFillColor);
    }

    const auto &oldFont = fontForType(oldType);
    const auto &newFont = fontForType(type);
    if (oldFont != newFont) {
        Q_EMIT fontChanged(newFont);
    }

    const auto &oldFontColor = fontColorForType(oldType);
    const auto &newFontColor = fontColorForType(type);
    if (oldFontColor != newFontColor) {
        Q_EMIT fontColorChanged(newFontColor);
    }

    const auto &oldShadow = typeHasShadow(oldType);
    const auto &newShadow = typeHasShadow(type);
    if (oldShadow != newShadow) {
        Q_EMIT shadowChanged(newShadow);
    }
}

void AnnotationTool::resetType()
{
    setType(AnnotationDocument::None);
}

AnnotationTool::Options AnnotationTool::options() const
{
    return m_options;
}

int AnnotationTool::strokeWidth() const
{
    return strokeWidthForType(m_type);
}

int AnnotationTool::strokeWidthForType(AnnotationDocument::EditActionType type) const
{
    switch (type) {
    case AnnotationDocument::FreeHand:
        return Settings::freehandStrokeWidth();
    case AnnotationDocument::Highlight:
        return Settings::highlighterStrokeWidth();
    case AnnotationDocument::Line:
        return Settings::lineStrokeWidth();
    case AnnotationDocument::Arrow:
        return Settings::arrowStrokeWidth();
    case AnnotationDocument::Rectangle:
        return Settings::rectangleStrokeWidth();
    case AnnotationDocument::Ellipse:
        return Settings::ellipseStrokeWidth();
    default: return 0;
    }
}

void AnnotationTool::setStrokeWidth(int width)
{
    if (!m_options.testFlag(Option::Stroke) || strokeWidth() == width) {
        return;
    }

    setStrokeWidthForType(width, m_type);
    Q_EMIT strokeWidthChanged(width);
}

void AnnotationTool::setStrokeWidthForType(int width, AnnotationDocument::EditActionType type)
{
    switch (type) {
    case AnnotationDocument::FreeHand:
        Settings::setFreehandStrokeWidth(width);
        break;
    case AnnotationDocument::Highlight:
        Settings::setHighlighterStrokeWidth(width);
        break;
    case AnnotationDocument::Line:
        Settings::setLineStrokeWidth(width);
        break;
    case AnnotationDocument::Arrow:
        Settings::setArrowStrokeWidth(width);
        break;
    case AnnotationDocument::Rectangle:
        Settings::setRectangleStrokeWidth(width);
        break;
    case AnnotationDocument::Ellipse:
        Settings::setEllipseStrokeWidth(width);
        break;
    default: break;
    }
}

void AnnotationTool::resetStrokeWidth()
{
    setStrokeWidth(defaultStrokeWidthForType(m_type));
}

QColor AnnotationTool::strokeColor() const
{
    return strokeColorForType(m_type);
}

QColor AnnotationTool::strokeColorForType(AnnotationDocument::EditActionType type) const
{
    switch (type) {
    case AnnotationDocument::FreeHand:
        return Settings::freehandStrokeColor();
    case AnnotationDocument::Highlight:
        return Settings::highlighterStrokeColor();
    case AnnotationDocument::Line:
        return Settings::lineStrokeColor();
    case AnnotationDocument::Arrow:
        return Settings::arrowStrokeColor();
    case AnnotationDocument::Rectangle:
        return Settings::rectangleStrokeColor();
    case AnnotationDocument::Ellipse:
        return Settings::ellipseStrokeColor();
    default: return Qt::transparent;
    }
}

void AnnotationTool::setStrokeColor(const QColor &color)
{
    if (!m_options.testFlag(Option::Stroke) || strokeColor() == color) {
        return;
    }

    setStrokeColorForType(color, m_type);

    Q_EMIT strokeColorChanged(color);
}

void AnnotationTool::setStrokeColorForType(const QColor &color,
                                           AnnotationDocument::EditActionType type)
{
    switch (type) {
    case AnnotationDocument::FreeHand:
        Settings::setFreehandStrokeColor(color);
        break;
    case AnnotationDocument::Highlight:
        Settings::setHighlighterStrokeColor(color);
        break;
    case AnnotationDocument::Line:
        Settings::setLineStrokeColor(color);
        break;
    case AnnotationDocument::Arrow:
        Settings::setArrowStrokeColor(color);
        break;
    case AnnotationDocument::Rectangle:
        Settings::setRectangleStrokeColor(color);
        break;
    case AnnotationDocument::Ellipse:
        Settings::setEllipseStrokeColor(color);
        break;
    default: break;
    }
}

void AnnotationTool::resetStrokeColor()
{
    setStrokeColor(defaultStrokeColorForType(m_type));
}

QColor AnnotationTool::fillColor() const
{
    return fillColorForType(m_type);
}

QColor AnnotationTool::fillColorForType(AnnotationDocument::EditActionType type) const
{
    switch (type) {
    case AnnotationDocument::Rectangle:
        return Settings::rectangleFillColor();
    case AnnotationDocument::Ellipse:
        return Settings::ellipseFillColor();
    case AnnotationDocument::Number:
        return Settings::numberFillColor();
    default: return Qt::transparent;
    }
}

void AnnotationTool::setFillColor(const QColor &color)
{
    if (!m_options.testFlag(Option::Fill) || fillColor() == color) {
        return;
    }

    setFillColorForType(color, m_type);
    Q_EMIT fillColorChanged(color);
}

void AnnotationTool::setFillColorForType(const QColor &color,
                                         AnnotationDocument::EditActionType type)
{
    switch (type) {
    case AnnotationDocument::Rectangle:
        Settings::setRectangleFillColor(color);
        break;
    case AnnotationDocument::Ellipse:
        Settings::setEllipseFillColor(color);
        break;
    case AnnotationDocument::Number:
        Settings::setNumberFillColor(color);
        break;
    default: break;
    }
}

void AnnotationTool::resetFillColor()
{
    setFillColor(defaultFillColorForType(m_type));
}

QFont AnnotationTool::font() const
{
    return fontForType(m_type);
}

QFont AnnotationTool::fontForType(AnnotationDocument::EditActionType type) const
{
    switch (type) {
    case AnnotationDocument::Text:
        return Settings::textFont();
    case AnnotationDocument::Number:
        return Settings::numberFont();
    default: return {};
    }
}

void AnnotationTool::setFont(const QFont &font)
{
    if (!m_options.testFlag(Option::Font) || this->font() == font) {
        return;
    }

    setFontForType(font, m_type);

    Q_EMIT fontChanged(font);
}

void AnnotationTool::setFontForType(const QFont &font, AnnotationDocument::EditActionType type)
{
    switch (type) {
    case AnnotationDocument::Text:
        Settings::setTextFont(font);
        break;
    case AnnotationDocument::Number:
        Settings::setNumberFont(font);
        break;
    default: break;
    }
}

void AnnotationTool::resetFont()
{
    setFont({});
}

QColor AnnotationTool::fontColor() const
{
    return fontColorForType(m_type);
}

QColor AnnotationTool::fontColorForType(AnnotationDocument::EditActionType type) const
{
    switch (type) {
    case AnnotationDocument::Text:
        return Settings::textFontColor();
    case AnnotationDocument::Number:
        return Settings::numberFontColor();
    default: return Qt::transparent;
    }
}

void AnnotationTool::setFontColor(const QColor &color)
{
    if (!m_options.testFlag(Option::Font) || fontColor() == color) {
        return;
    }

    setFontColorForType(color, m_type);
    Q_EMIT fontColorChanged(color);
}

void AnnotationTool::setFontColorForType(const QColor &color,
                                         AnnotationDocument::EditActionType type)
{
    switch (type) {
    case AnnotationDocument::Text:
        Settings::setTextFontColor(color);
        break;
    case AnnotationDocument::Number:
        Settings::setNumberFontColor(color);
        break;
    default: break;
    }
}

void AnnotationTool::resetFontColor()
{
    setFontColor(defaultFontColorForType(m_type));
}

int AnnotationTool::number() const
{
    return m_number;
}

void AnnotationTool::setNumber(int number)
{
    if (m_number == number) {
        return;
    }

    m_number = number;

    Q_EMIT numberChanged(number);
}

void AnnotationTool::resetNumber()
{
    setNumber(1);
}

bool AnnotationTool::typeHasShadow(AnnotationDocument::EditActionType type) const
{
    switch (type) {
    case AnnotationDocument::FreeHand:
        return Settings::freehandShadow();
    case AnnotationDocument::Line:
        return Settings::lineShadow();
    case AnnotationDocument::Arrow:
        return Settings::arrowShadow();
    case AnnotationDocument::Rectangle:
        return Settings::rectangleShadow();
    case AnnotationDocument::Ellipse:
        return Settings::ellipseShadow();
    case AnnotationDocument::Text:
        return Settings::textShadow();
    case AnnotationDocument::Number:
        return Settings::numberShadow();
    default:
        return false;
    }
}

bool AnnotationTool::hasShadow() const
{
    return typeHasShadow(m_type);
}

void AnnotationTool::setTypeHasShadow(AnnotationDocument::EditActionType type, bool shadow)
{
    switch (type) {
    case AnnotationDocument::FreeHand:
        Settings::setFreehandShadow(shadow);
        break;
    case AnnotationDocument::Line:
        Settings::setLineShadow(shadow);
        break;
    case AnnotationDocument::Arrow:
        Settings::setArrowShadow(shadow);
        break;
    case AnnotationDocument::Rectangle:
        Settings::setRectangleShadow(shadow);
        break;
    case AnnotationDocument::Ellipse:
        Settings::setEllipseShadow(shadow);
        break;
    case AnnotationDocument::Text:
        Settings::setTextShadow(shadow);
        break;
    case AnnotationDocument::Number:
        Settings::setNumberShadow(shadow);
        break;
    default:
        break;
    }
}

void AnnotationTool::setShadow(bool shadow)
{
    if (!m_options.testFlag(Option::Shadow) || hasShadow() == shadow) {
        return;
    }

    setTypeHasShadow(m_type, shadow);
    Q_EMIT shadowChanged(shadow);
}

void AnnotationTool::resetShadow()
{
    setShadow(true);
}

//////////////////////////

SelectedActionWrapper::SelectedActionWrapper(AnnotationDocument *document)
    : QObject(document)
    , m_document(document)
{
}

SelectedActionWrapper::~SelectedActionWrapper()
{
}

EditAction *SelectedActionWrapper::editAction() const
{
    return m_editAction;
}

void SelectedActionWrapper::setEditAction(EditAction *action)
{
    if (m_editAction == action
        || (action && (action->type() == AnnotationDocument::None
                       || m_document->m_undoStack.contains(action->replacedBy())))) {
        return;
    }

    // commit if valid, restore if invalid
    auto copy = m_actionCopy.get();
    if (m_editAction && copy && *m_editAction != copy
        && !m_document->m_undoStack.contains(m_editAction->replacedBy())
    ) {
        if (m_editAction->isValid() && copy->isValid()) {
            auto replacement = m_editAction->createReplacement();
            m_editAction->copyFrom(copy);
            m_document->addAction(replacement);
        } else if (copy->isValid()) {
            m_editAction->copyFrom(copy);
            m_document->emitRepaintNeededUnlessEmpty(m_editAction->lastUpdateArea());
        } else if (!m_editAction->isValid()) {
            if (m_document->m_undoStack.contains(m_editAction)) {
                m_document->m_undoStack.removeAll(m_editAction);
                Q_EMIT m_document->undoStackDepthChanged();
            }
            if (m_document->m_redoStack.contains(m_editAction)) {
                m_document->m_redoStack.removeAll(m_editAction);
                Q_EMIT m_document->redoStackDepthChanged();
            }
            auto oldAction = m_editAction;
            m_editAction = nullptr;
            m_document->permanentlyDeleteAction(oldAction);
        } else if (m_editAction->isValid() && m_type == AnnotationDocument::Text) {
            // we don't emit this until an action that was added is valid,
            // so we need to emit here for text.
            Q_EMIT m_document->undoStackDepthChanged();
        }
    }
    m_editAction = action;
    if (m_editAction) {
        m_type = action->type();
        m_actionCopy.reset(m_editAction->createCopy());
    } else {
        m_type = AnnotationDocument::None;
        m_actionCopy.reset();
    }
    m_options = optionsForType(m_type);
    // all bindings using the selectedAction property should be re-evalulated when emitted
    Q_EMIT m_document->selectedActionWrapperChanged();
}

bool SelectedActionWrapper::isValid() const
{
    return m_editAction && m_editAction->isValid();
}

AnnotationDocument::EditActionType SelectedActionWrapper::type() const
{
    return m_type;
}

AnnotationTool::Options SelectedActionWrapper::options() const
{
    return m_options;
}

int SelectedActionWrapper::strokeWidth() const
{
    return m_options.testFlag(AnnotationTool::Stroke) ?
        m_editAction->strokeWidth() : defaultStrokeWidthForType(m_type);
}

void SelectedActionWrapper::setStrokeWidth(int width)
{
    if (!m_options.testFlag(AnnotationTool::Stroke) || m_editAction->strokeWidth() == width) {
        return;
    }
    m_editAction->setStrokeWidth(width);
    Q_EMIT strokeWidthChanged();
    Q_EMIT visualGeometryChanged();
    m_document->emitRepaintNeededUnlessEmpty(m_editAction->lastUpdateArea());
}

QColor SelectedActionWrapper::strokeColor() const
{
    return m_options.testFlag(AnnotationTool::Stroke) ?
        m_editAction->strokeColor() : defaultStrokeColorForType(m_type);
}

void SelectedActionWrapper::setStrokeColor(const QColor &color)
{
    if (!m_options.testFlag(AnnotationTool::Stroke) || m_editAction->strokeColor() == color) {
        return;
    }
    m_editAction->setStrokeColor(color);
    Q_EMIT strokeColorChanged();
    m_document->emitRepaintNeededUnlessEmpty(m_editAction->lastUpdateArea());
}

QColor SelectedActionWrapper::fillColor() const
{
    return m_options.testFlag(AnnotationTool::Fill) ?
        m_editAction->fillColor() : defaultFillColorForType(m_type);
}

void SelectedActionWrapper::setFillColor(const QColor &color)
{
    if (!m_options.testFlag(AnnotationTool::Fill) || m_editAction->fillColor() == color) {
        return;
    }
    m_editAction->setFillColor(color);
    Q_EMIT fillColorChanged();
    m_document->emitRepaintNeededUnlessEmpty(m_editAction->lastUpdateArea());
}

QFont SelectedActionWrapper::font() const
{
    return m_options.testFlag(AnnotationTool::Font) ?
        m_editAction->font() : QFont();
}

void SelectedActionWrapper::setFont(const QFont &font)
{
    if (!m_options.testFlag(AnnotationTool::Font) || m_editAction->font() == font) {
        return;
    }
    auto oldRect = m_editAction->visualGeometry();
    m_editAction->setFont(font);
    Q_EMIT fontChanged();
    if (oldRect != m_editAction->visualGeometry()) {
        Q_EMIT visualGeometryChanged();
    }
    m_document->emitRepaintNeededUnlessEmpty(m_editAction->lastUpdateArea());
}

QColor SelectedActionWrapper::fontColor() const
{
    return m_options.testFlag(AnnotationTool::Font) ?
        m_editAction->fontColor() : defaultFontColorForType(m_type);
}

void SelectedActionWrapper::setFontColor(const QColor &color)
{
    if (!m_options.testFlag(AnnotationTool::Font) || fontColor() == color) {
        return;
    }
    m_editAction->setFontColor(color);
    Q_EMIT fontColorChanged();
    m_document->emitRepaintNeededUnlessEmpty(m_editAction->lastUpdateArea());
}

int SelectedActionWrapper::number() const
{
    return m_type == AnnotationDocument::Number ? m_editAction->number() : 1;
}

void SelectedActionWrapper::setNumber(int number)
{
    if (m_type != AnnotationDocument::Number || m_editAction->number() == number) {
        return;
    }
    // NumberActions only set size when constructed
    m_editAction->setNumber(number);
    Q_EMIT numberChanged();
    m_document->emitRepaintNeededUnlessEmpty(m_editAction->lastUpdateArea());
}

QString SelectedActionWrapper::text() const
{
    if (m_type == AnnotationDocument::Text) {
        auto *ta = static_cast<TextAction *>(m_editAction);
        return ta->text();
    } else {
        return QString();
    }
}

void SelectedActionWrapper::setText(const QString &text)
{
    if (m_type != AnnotationDocument::Text) {
        return;
    }

    auto *ta = static_cast<TextAction *>(m_editAction);
    if (text != ta->text()) {
        auto oldRect = m_editAction->visualGeometry();
        ta->setText(text);
        Q_EMIT textChanged();
        if (oldRect != m_editAction->visualGeometry()) {
            Q_EMIT visualGeometryChanged();
        }
        m_document->emitRepaintNeededUnlessEmpty(ta->lastUpdateArea());
    }
}

QRectF SelectedActionWrapper::visualGeometry() const
{
    return m_editAction ? m_editAction->visualGeometry() : QRectF();
}

void SelectedActionWrapper::setVisualGeometry(const QRectF &geom)
{
    if (!m_editAction || m_editAction->visualGeometry() == geom) {
        return;
    }
    // determine when to duplicate in ResizeHandles.qml
    m_editAction->setVisualGeometry(geom);
    Q_EMIT visualGeometryChanged();
    m_document->emitRepaintNeededUnlessEmpty(m_editAction->lastUpdateArea());
}

bool SelectedActionWrapper::hasShadow() const
{
    return m_editAction && m_editAction->supportsShadow() && m_editAction->hasShadow();
}

void SelectedActionWrapper::setShadow(bool shadow)
{
    if (!m_editAction || !m_editAction->supportsShadow() || m_editAction->hasShadow() == shadow) {
        return;
    }

    m_editAction->setShadow(shadow);
    Q_EMIT shadowChanged();
    m_document->emitRepaintNeededUnlessEmpty(m_editAction->lastUpdateArea());
}

void SelectedActionWrapper::commitChanges()
{
    auto copy = m_actionCopy.get();
    if (!m_editAction || !m_editAction->isValid() || !copy || !copy->isValid() /*|| *m_editAction == copy*/
        || m_document->m_undoStack.contains(m_editAction->replacedBy())) {
        return;
    }
    auto replacement = m_editAction->createReplacement(); // make replacement using current data
    m_editAction->copyFrom(copy); // restore old data to current action
    m_actionCopy.reset(); // delete action copy to make restoring in setEditAction easier
    m_document->addAction(replacement); // do the replacement
    m_editAction = replacement;

    for (auto *ea : std::as_const(m_document->m_undoStack)) {
        if (ea->type() == AnnotationDocument::Blur || ea->type() == AnnotationDocument::Pixelate) {
            auto *sa = static_cast<ShapeAction *>(ea);
            sa->invalidateCache();
        }
    }
    Q_EMIT m_document->selectedActionWrapperChanged();
    Q_EMIT m_document->repaintNeeded(); // todo: boundingrect?
}

QDebug operator<<(QDebug debug, const SelectedActionWrapper *saw)
{
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << "SelectedActionWrapper(";
    if (!saw) {
        return debug << "0x0)";
    }
    debug << (const void *)saw;
    debug << ", editAction=" << saw->editAction();
    debug << ')';
    return debug;
}

//////////////////////////

AnnotationDocument::AnnotationDocument(QObject *parent)
    : QObject(parent)
    , m_tool(new AnnotationTool(this))
    , m_selectedActionWrapper(new SelectedActionWrapper(this))
{
}

AnnotationDocument::~AnnotationDocument()
{
    qDeleteAll(m_undoStack);
    m_undoStack.clear();
    qDeleteAll(m_redoStack);
    m_redoStack.clear();
    clearImages();
}

AnnotationTool *AnnotationDocument::tool() const
{
    return m_tool;
}

SelectedActionWrapper *AnnotationDocument::selectedActionWrapper() const
{
    return m_selectedActionWrapper;
}

void AnnotationDocument::clear()
{
    clearAnnotations();
    clearImages();
}

void AnnotationDocument::cropCanvas(const QRectF &cropRect)
{
    QVector<EditAction *> filteredUndo;
    QVector<EditAction *> filteredRedo;
    for (auto *ea : std::as_const(m_undoStack)) {
        if (ea->visualGeometry().intersects(cropRect)) {
            ea->translate(-cropRect.topLeft());
            if (ea->type() == AnnotationDocument::Blur || ea->type() == AnnotationDocument::Pixelate) {
                static_cast<ShapeAction *>(ea)->invalidateCache();
            }
            filteredUndo << ea;
        } else {
            delete ea;
        }
    }
    m_undoStack = filteredUndo;

    for (auto *ea : std::as_const(m_redoStack)) {
        if (ea->visualGeometry().intersects(cropRect)) {
            ea->translate(-cropRect.topLeft());
            filteredRedo << ea;
        } else {
            delete ea;
        }
    }
    m_redoStack = filteredRedo;

    for (auto &img : m_canvasImages) {
        img.rect.setTopLeft(img.rect.topLeft() - cropRect.topLeft());
    }

    m_canvasSize = cropRect.size();

    Q_EMIT canvasSizeChanged();
    Q_EMIT undoStackDepthChanged();
    Q_EMIT redoStackDepthChanged();
    Q_EMIT repaintNeeded();
}

QSizeF AnnotationDocument::canvasSize() const
{
    return m_canvasSize;
}

void AnnotationDocument::addImage(const CanvasImage &canvasImage)
{
    m_canvasImages << canvasImage;

    QRectF rect;
    for (const auto &img : qAsConst(m_canvasImages)) {
        rect = rect.united(img.rect);
    }
    m_canvasSize = rect.size();

    Q_EMIT canvasSizeChanged();
    Q_EMIT repaintNeeded();
}

QVector<CanvasImage> AnnotationDocument::canvasImages() const
{
    return m_canvasImages;
}

void AnnotationDocument::clearImages()
{
    m_canvasImages.clear();
    m_canvasImages.squeeze();
    m_canvasSize = QSizeF();
    Q_EMIT canvasSizeChanged();
    Q_EMIT repaintNeeded();
}

void AnnotationDocument::clearAnnotations()
{
    qDeleteAll(m_undoStack);
    m_undoStack.clear();
    m_undoStack.squeeze();
    qDeleteAll(m_redoStack);
    m_redoStack.clear();
    m_redoStack.squeeze();
    m_tool->resetType();
    m_tool->resetNumber();
    deselectAction();
    Q_EMIT undoStackDepthChanged();
    Q_EMIT redoStackDepthChanged();
    Q_EMIT repaintNeeded();
}

inline QPointF zoomPoint(const QPointF &pos, qreal zoomFactor)
{
    return QPointF(pos.x() * zoomFactor, pos.y() * zoomFactor);
}

inline QSizeF zoomSize(const QSizeF &size, qreal zoomFactor)
{
    return QSizeF(size.width() * zoomFactor, size.height() * zoomFactor);
}

inline QRectF zoomRect(const QRectF &rect, qreal zoomFactor)
{
    return QRectF(rect.x() * zoomFactor, rect.y() * zoomFactor, rect.width() * zoomFactor, rect.height() * zoomFactor);
}

void AnnotationDocument::paint(QPainter *painter, const QRectF &viewPort, qreal zoomFactor) const
{
    static QList<EditAction *> stopAtAction = QList<EditAction *>();
    const qreal scale = painter->transform().m11();

    for (const auto &img : qAsConst(m_canvasImages)) {
        if (viewPort.intersects(img.rect)) {
            // More High quality scale down
            auto scaledImg = img.image.scaled(zoomSize(img.image.size(), zoomFactor).toSize(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            scaledImg.setDevicePixelRatio(img.image.devicePixelRatio());
            painter->drawImage(zoomPoint((img.rect.topLeft() - viewPort.topLeft()) * img.image.devicePixelRatio(), zoomFactor).toPoint(), scaledImg);
        }
    }

    painter->scale(zoomFactor, zoomFactor);
    painter->setRenderHints({QPainter::Antialiasing, QPainter::TextAntialiasing});
    for (auto *ea : m_undoStack) {
        if (!stopAtAction.isEmpty() && ea == stopAtAction.last()) {
            painter->scale(scale, scale);
            return;
        }

        if (!isActionVisible(ea, zoomRect(viewPort, 1 / zoomFactor))) {
            continue;
        }

        bool offsetStroke = scale == 1 && (ea->strokeWidth() % 2 || ea->strokeWidth() == 0);
        qreal penWidth = ea->strokeWidth();
        // QPainter will force pen width to 1 if it is set to 0 and strokes are being drawn.
        // QPainter doesn't draw exactly 1px strokes nicely.
        if (penWidth == 0 || penWidth == 1) {
            penWidth = 1.001;
        }
        QPen pen(ea->strokeColor(), penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);

        // Draw the shadow if existent
        if (ea->hasShadow()) {
            QImage shadow = shapeShadow(ea, painter->deviceTransform().m11() / zoomFactor);
            painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
            painter->drawImage(QRectF(ea->visualGeometry().translated(-viewPort.topLeft()) + ea->shadowMargins()).topLeft(), shadow);
            painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
        }

        switch (ea->type()) {
        case FreeHand:
        case Highlight: {
            auto *fha = static_cast<FreeHandAction *>(ea);
            if (ea->type() == Highlight) {
                painter->setCompositionMode(QPainter::CompositionMode_Darken);
            }
            QPointF offsetPos;
            if (offsetStroke) {
                offsetPos = {0.5, 0.5};
            }
            auto path = fha->path().translated(-viewPort.topLeft() + offsetPos);
            if (path.isEmpty()) {
                auto start = path.elementAt(0);
                // ensure a single point freehand event is visible
                path.lineTo(start.x + 0.0001, start.y);
            }
            painter->drawPath(path);
            break;
        }
        case Line:
        case Arrow: {
            auto *la = static_cast<LineAction *>(ea);
            QPointF offsetPos;
            if (offsetStroke) {
                offsetPos = {0.5, 0.5};
            }
            const auto &line = la->line().translated(-viewPort.topLeft() + offsetPos);

            painter->drawLine(line);
            if (la->type() == Arrow) {
                painter->drawPolyline(la->arrowHeadPolygon(line));
            }
            break;
        }
        case Rectangle:
        case Ellipse: {
            auto *sa = static_cast<ShapeAction *>(ea);
            painter->setPen(sa->strokeWidth() > 0 ? pen : Qt::NoPen);
            painter->setBrush(sa->fillColor());
            QMarginsF offsetMargins;
            if (offsetStroke && sa->strokeWidth() > 0) {
                qreal offset = 0.5;
                offsetMargins = {-offset, -offset, -offset, -offset};
            }
            const auto rect = sa->geometry().translated(-viewPort.topLeft()) + offsetMargins;
            switch (sa->type()) {
            case Rectangle:
                painter->drawRect(rect);
                break;
            case Ellipse:
                painter->drawEllipse(rect);
                break;
            default:
                break;
            }
            break;
        }
        case Blur: {
            auto *sa = static_cast<ShapeAction *>(ea);
            const QRectF &targetRect = sa->geometry().normalized().translated(-viewPort.topLeft());
            const qreal factor = 2 * qGuiApp->devicePixelRatio(); // take the maximum scale factor of any screen
            const qreal dpr = 1.0 / factor;
            if (sa->backingStoreCache().isNull() || sa->backingStoreCache().devicePixelRatio() != painter->deviceTransform().m11()) {
                stopAtAction << ea;
                sa->backingStoreCache() = renderToImage(QRectF(QPointF(0, 0), canvasSize()), dpr);
                sa->backingStoreCache().setDevicePixelRatio(painter->deviceTransform().m11());
                stopAtAction.pop_back();
                // With more scaling, blur more
                sa->backingStoreCache() = fastPseudoBlur(sa->backingStoreCache(), 4, painter->deviceTransform().m11());
            }
            painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
            painter->drawImage(targetRect, sa->backingStoreCache(), zoomRect(sa->geometry(), dpr).normalized());
            painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
            break;
        }
        case Pixelate: {
            auto *sa = static_cast<ShapeAction *>(ea);
            const QRectF &targetRect = sa->geometry().normalized().translated(-viewPort.topLeft());
            const qreal factor = 4;
            const qreal dpr = 1.0 / factor;
            if (sa->backingStoreCache().isNull()) {
                stopAtAction << ea;
                sa->backingStoreCache() = renderToImage(QRectF(QPointF(0, 0), canvasSize()), dpr);

                stopAtAction.pop_back();
            }
            painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
            painter->drawImage(targetRect, sa->backingStoreCache(), zoomRect(sa->geometry(), dpr).normalized());
            break;
        }
        case Text: {
            auto *ta = static_cast<TextAction *>(ea);
            painter->setFont(ta->font());
            painter->setPen(ta->fontColor());
            QPointF baselineOffset = {0, QFontMetricsF(ta->font()).ascent()};
            painter->drawText(ta->startPoint() - viewPort.topLeft() + baselineOffset, ta->text());
            break;
        }
        case Number: {
            auto *na = static_cast<NumberAction *>(ea);
            const auto rect = na->boundingRect().translated(-viewPort.topLeft());
            painter->setBrush(na->fillColor());
            painter->setFont(na->font());
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(rect);
            painter->setPen(na->fontColor());
            painter->drawText(rect, Qt::AlignCenter, QString::number(na->number()));
            break;
        }
        default:
            break;
        }
    }
    painter->scale(scale, scale);
}

QImage AnnotationDocument::renderToImage(const QRectF &viewPort, qreal devicePixelRatio) const
{
    QImage img(viewPort.width() /* devicePixelRatio*/, viewPort.height() /* devicePixelRatio*/, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    // Makes pixelate and blur look better
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.scale(devicePixelRatio, devicePixelRatio);
    paint(&p, viewPort);
    p.end();

    return img;
}

int AnnotationDocument::undoStackDepth() const
{
    return m_undoStack.count();
}

int AnnotationDocument::redoStackDepth() const
{
    return m_redoStack.count();
}

bool AnnotationDocument::isLastActionInvalid() const
{
    return !m_undoStack.isEmpty() && !m_undoStack.last()->isValid();
}

void AnnotationDocument::permanentlyDeleteLastAction()
{
    if (m_undoStack.isEmpty()) {
        return;
    }
    permanentlyDeleteAction(m_undoStack.last());
}

EditAction *AnnotationDocument::actionAtPoint(const QPointF &point) const
{
    for (int i = m_undoStack.count() - 1; i >= 0; --i) {
        auto action = m_undoStack[i];
        if (isActionVisible(action) && action->visualGeometry().contains(point)) {
            return action;
        }
    }
    return nullptr;
}

QRectF AnnotationDocument::visualGeometryAtPoint(const QPointF &point) const
{
    auto action = actionAtPoint(point);
    if (!action) {
        return {};
    }
    return action->visualGeometry();
}

void AnnotationDocument::undo()
{
    if (m_undoStack.isEmpty()) {
        return;
    }

    auto *action = m_undoStack.last();
    auto *replacedAction = action->replaces();
    int undoCount = m_undoStack.size();
    bool isSelected = action == m_selectedActionWrapper->editAction();
    bool replacesEarlierAction = undoCount > 1 && replacedAction
                              && replacedAction == m_undoStack[undoCount - 2];

    if (isSelected && !replacesEarlierAction) {
        deselectAction();
    }

    if (m_undoStack.isEmpty()) {
        // deselectAction() could have deleted the last action in the stack
        return;
    }

    auto updateRect = action->lastUpdateArea();
    if (action->replaces()) {
        updateRect = updateRect.united(action->replaces()->lastUpdateArea());
    }
    m_redoStack << action;
    m_undoStack.pop_back();

    if (action->type() == AnnotationDocument::Number) {
        auto na = static_cast<NumberAction *>(action);
        m_tool->setNumber(na->number());
    }

    if (isSelected && replacesEarlierAction) {
        m_selectedActionWrapper->setEditAction(replacedAction);
    }

    for (auto action : std::as_const(m_undoStack)) {
        if (isActionVisible(action, updateRect)) {
            updateRect = updateRect.united(action->lastUpdateArea());
        }
        if (action->type() == AnnotationDocument::Blur
            || action->type() == AnnotationDocument::Pixelate) {
            auto *sa = static_cast<ShapeAction *>(action);
            sa->invalidateCache();
        }
    }
    Q_EMIT undoStackDepthChanged();
    Q_EMIT redoStackDepthChanged();
    emitRepaintNeededUnlessEmpty(updateRect);
}

void AnnotationDocument::redo()
{
    if (m_redoStack.isEmpty()) {
        return;
    }

    auto *action = m_redoStack.last();
    auto *replacedAction = action->replaces();
    int undoCount = m_undoStack.size();
    bool isReplacedSelected = replacedAction == m_selectedActionWrapper->editAction();
    bool replacesEarlierAction = undoCount > 0 && replacedAction
                              && replacedAction == m_undoStack[undoCount - 1];

    if (isReplacedSelected && !replacesEarlierAction) {
        deselectAction();
    }

    auto updateRect = action->lastUpdateArea();
    if (action->replaces()) {
        updateRect = updateRect.united(action->replaces()->lastUpdateArea());
    }
    m_undoStack << action;
    m_redoStack.pop_back();

    if (action->type() == AnnotationDocument::Number) {
        auto na = static_cast<NumberAction *>(action);
        m_tool->setNumber(na->number() + 1);
    }

    if (isReplacedSelected && replacesEarlierAction) {
        m_selectedActionWrapper->setEditAction(action);
    }

    for (auto action : std::as_const(m_undoStack)) {
        if (isActionVisible(action, updateRect)) {
            updateRect = updateRect.united(action->lastUpdateArea());
        }
    }
    Q_EMIT undoStackDepthChanged();
    Q_EMIT redoStackDepthChanged();
    emitRepaintNeededUnlessEmpty(updateRect);
}

void AnnotationDocument::beginAction(const QPointF &point)
{
    QRectF updateRect;
    // if the last action was not valid, discard it (for instance a rectangle with 0 size)
    if (isLastActionInvalid()) {
        QSignalBlocker blocker(this);
        m_selectedActionWrapper->blockSignals(m_tool->type() == Text);
        auto lastAction = m_undoStack.last();
        updateRect = lastAction->lastUpdateArea();
        permanentlyDeleteAction(lastAction);
        m_selectedActionWrapper->blockSignals(false);
    }

    EditAction *action = nullptr;

    switch (m_tool->type()) {
    case FreeHand:
    case Highlight: {
        action = new FreeHandAction(m_tool, point);
        addAction(action);
        break;
    }
    case Line:
    case Arrow: {
        action = new LineAction(m_tool, point);
        addAction(action);
        break;
    }
    case Rectangle:
    case Ellipse:
    case Blur:
    case Pixelate: {
        action = new ShapeAction(m_tool, point);
        addAction(action);
        break;
    }
    case Text: {
        action = new TextAction(m_tool, point);
        addAction(action);
        m_selectedActionWrapper->setEditAction(action);
        break;
    }
    case Number: {
        action = new NumberAction(m_tool, point);
        addAction(action);
        m_tool->setNumber(m_tool->number() + 1);
        break;
    }
    default: return;
    }

    if (action) {
        if (action->supportsShadow()) {
            action->setShadow(m_tool->hasShadow());
        }
        updateRect = updateRect.united(action->lastUpdateArea());
    }
    emitRepaintNeededUnlessEmpty(updateRect);
}

void AnnotationDocument::continueAction(const QPointF &point, ContinueOptions options)
{
    auto action = m_undoStack.isEmpty() ? nullptr : m_undoStack.last();
    if (!action || action->type() == None || m_tool->type() == ChangeAction) {
        return;
    }

    auto isSelectedAction = m_selectedActionWrapper->editAction() == action;
    QRectF oldSelectedActionVisualGeometry;
    bool wasValid = action->isValid();
    if (isSelectedAction) {
        oldSelectedActionVisualGeometry = action->visualGeometry();
    }

    switch (action->type()) {
    case FreeHand:
    case Highlight: {
        auto *fha = static_cast<FreeHandAction *>(action);
        fha->addPoint(point);
        emitRepaintNeededUnlessEmpty(fha->lastUpdateArea());
        break;
    }
    case Line:
    case Arrow: {
        auto *la = static_cast<LineAction *>(action);
        QPointF endPoint = point;
        if (options & ContinueOption::SnapAngle) {
            QPointF posDiff = point - la->line().p1();
            if (qAbs(posDiff.x()) / 1.5 > qAbs(posDiff.y())) {
                // horizontal
                endPoint.setY(la->line().p1().y());
            } else if (qAbs(posDiff.x()) < qAbs(posDiff.y()) / 1.5) {
                // vertical
                endPoint.setX(la->line().p1().x());
            } else {
                // diagonal when 1/3 in between horizontal and vertical
                auto xSign = std::copysign(1.0, posDiff.x());
                auto ySign = std::copysign(1.0, posDiff.y());
                qreal max = qMax(qAbs(posDiff.x()), qAbs(posDiff.y()));
                endPoint = la->line().p1() + QPointF(max * xSign, max * ySign);
            }
        }
        la->setEndPoint(endPoint);
        emitRepaintNeededUnlessEmpty(la->lastUpdateArea());
        break;
    }
    case Rectangle:
    case Ellipse:
    case Blur:
    case Pixelate: {
        auto *sa = static_cast<ShapeAction *>(action);
        const auto &geometry = sa->geometry();
        QRectF rect(geometry.topLeft(), point);
        if (options & ContinueOption::SnapAngle) {
            auto wSign = std::copysign(1.0, rect.width());
            auto hSign = std::copysign(1.0, rect.height());
            qreal max = qMax(qAbs(rect.width()), qAbs(rect.height()));
            rect.setSize({max * wSign, max * hSign});
        }
        if (options & ContinueOption::CenterResize) {
            rect.moveCenter(geometry.center());
        }
        sa->setGeometry(rect);
        emitRepaintNeededUnlessEmpty(sa->lastUpdateArea());
        break;
    }
    case Text: {
        auto visualGeometry = action->visualGeometry();
        visualGeometry.moveTo(point.x(), point.y() - visualGeometry.height() / 2);
        if (action == m_selectedActionWrapper->editAction()) {
            m_selectedActionWrapper->setVisualGeometry(visualGeometry);
        } else {
            action->setVisualGeometry(visualGeometry);
        }
        emitRepaintNeededUnlessEmpty(action->lastUpdateArea());
        break;
    }
    case Number: {
        auto visualGeometry = action->visualGeometry();
        visualGeometry.moveCenter(point);
        action->setVisualGeometry(visualGeometry);
        emitRepaintNeededUnlessEmpty(action->lastUpdateArea());
        break;
    }
    default: return;
    }

    if (action->isValid() && !wasValid) {
        // Notify undo count change here if is now valid since
        // beginAction doesn't emit if action was invalid.
        Q_EMIT undoStackDepthChanged();
    }
    if (isSelectedAction && oldSelectedActionVisualGeometry != action->visualGeometry()) {
        Q_EMIT m_selectedActionWrapper->visualGeometryChanged();
    }
}

void AnnotationDocument::finishAction()
{
    auto action = m_undoStack.isEmpty() ? nullptr : m_undoStack.last();
    if (!action || action->type() == None || m_tool->type() == ChangeAction) {
        return;
    }

    if (action->type() == AnnotationDocument::FreeHand) {
        auto *fa = static_cast<FreeHandAction *>(action);
        fa->makeSmooth();
        Q_EMIT repaintNeeded(fa->visualGeometry());
    }
}

void AnnotationDocument::selectAction(const QPointF &point)
{
    m_selectedActionWrapper->setEditAction(actionAtPoint(point));
}

void AnnotationDocument::deselectAction()
{
    m_selectedActionWrapper->setEditAction(nullptr);
}

void AnnotationDocument::deleteSelectedAction()
{
    auto *action = m_selectedActionWrapper->editAction();
    if (!action || m_undoStack.contains(action->replacedBy())) {
        return;
    }

    deselectAction();
    if (!action->isValid()) {
        action = action->replaces();
        const int i = m_undoStack.lastIndexOf(action);
        m_undoStack.remove(i);
    }
    QRectF updateRect;
    if (action && action->isValid()) {
        // add undoable action for representing the deletion of an action
        m_undoStack << new DeleteAction(action);
        updateRect = action->lastUpdateArea();
    } else {
        const int i = m_undoStack.lastIndexOf(action);
        m_undoStack.remove(i);
    }

    Q_EMIT undoStackDepthChanged();
    clearRedoStack();
    emitRepaintNeededUnlessEmpty(updateRect);
}

void AnnotationDocument::addAction(EditAction *action)
{
    if (!action || action->replacedBy() || m_undoStack.contains(action)) {
        return;
    }
    m_undoStack << action;

    Q_EMIT undoStackDepthChanged();
    clearRedoStack();
}

void AnnotationDocument::permanentlyDeleteAction(EditAction *action)
{
    const int i = m_undoStack.lastIndexOf(action);
    if (i == -1) {
        return;
    }

    const int oldSize = m_undoStack.size();
    auto *selectedAction = m_selectedActionWrapper->editAction();
    if (selectedAction == action) {
        deselectAction();
    }
    // If deselecting the action already caused a removal, do nothing
    if (oldSize != m_undoStack.size()) {
        return;
    }

    m_undoStack.remove(i);
    Q_EMIT undoStackDepthChanged();
    emitRepaintNeededUnlessEmpty(action->lastUpdateArea());
    delete action;
}

void AnnotationDocument::clearRedoStack()
{
    int oldRedoCount = m_redoStack.count();
    qDeleteAll(m_redoStack);
    m_redoStack.clear();
    if (oldRedoCount != m_redoStack.count()) {
        Q_EMIT redoStackDepthChanged();
    }
}

bool AnnotationDocument::isActionVisible(EditAction *action, const QRectF &rect) const
{
    return action && action->isVisible()
            && (!action->replacedBy() || !m_undoStack.contains(action->replacedBy()))
            && (rect.isEmpty() || action->visualGeometry().intersects(rect));
}

void AnnotationDocument::emitRepaintNeededUnlessEmpty(const QRectF &area)
{
    if (!area.isEmpty()) {
        Q_EMIT repaintNeeded(area);
    }
}

#include <moc_AnnotationDocument.cpp>
