/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "EditAction.h"
#include "Utils.h"

#include <QDebug>
#include <QFontMetrics>
#include <cmath>
#include <cstdint>

// This is for adding margins to a QRectF that still has
// the expected visual shape change for negative sizes.
constexpr static inline QRectF visualMarginsAdded(const QRectF &rect, const QMarginsF &margins)
{
    QRectF newRect;
    if (rect.left() > rect.right()) {
        newRect.setLeft(rect.left() + margins.right());
        newRect.setRight(rect.right() - margins.left());
    } else {
        newRect.setLeft(rect.left() - margins.left());
        newRect.setRight(rect.right() + margins.right());
    }
    if (rect.top() > rect.bottom()) {
        newRect.setTop(rect.top() + margins.bottom());
        newRect.setBottom(rect.bottom() - margins.top());
    } else {
        newRect.setTop(rect.top() - margins.top());
        newRect.setBottom(rect.bottom() + margins.bottom());
    }
    return newRect;
}

constexpr static inline qreal mapValue(qreal scale, qreal elementValue,
                                       qreal oldRectValue, qreal newRectValue)
{
    qreal mappedFromOldRect = elementValue - oldRectValue;
    return scale * mappedFromOldRect + newRectValue;
}

/**
 * NOTE: There are 2 ways to translate element positions.
 *
 * This results in smooth translations:
 * ```
 * qreal xMappedFromOldRect = element.x - oldRect.x();
 * element.x = sx * xMappedFromOldRect + geom.x();
 * ```
 *
 * This results in translations that jitter around a lot:
 * ```
 * qreal dx = geom.x() - oldRect.x(); // m11
 * element.x = sx * element.x + dx;
 * ```
 *
 * QTransform::map() uses the jittery technique by design, so we don't use it.
 */
static inline QPainterPath mapPathToRect(const QRectF &newRect, const QPainterPath &oldPath)
{
    const auto oldRect = oldPath.controlPointRect();
    const qreal xScale = newRect.width() / oldRect.width(); // m11
    const qreal yScale = newRect.height() / oldRect.height(); // m21
    // use qIsFinite to deal with divide by zero
    const bool validXScale = qIsFinite(xScale) && !qFuzzyIsNull(xScale);
    const bool validYScale = qIsFinite(yScale) && !qFuzzyIsNull(yScale);

    if (!validXScale && !validYScale) {
        return oldPath;
    }

    QPainterPath newPath = oldPath;
    const qreal oldRectX = oldRect.x();
    const qreal oldRectY = oldRect.y();
    const qreal newRectX = newRect.x();
    const qreal newRectY = newRect.y();

    for (int i = 0; i < newPath.elementCount(); ++i) {
        auto element = newPath.elementAt(i);
        if (validXScale) {
            element.x = mapValue(xScale, element.x, oldRectX, newRectX);
        }
        if (validYScale) {
            element.y = mapValue(yScale, element.y, oldRectY, newRectY);
        }
        newPath.setElementPositionAt(i, element.x, element.y);
    }
    return newPath;
}

static inline QLineF mapLineToRect(const QRectF &newRect, const QLineF &oldLine)
{
    const auto oldRect = QRectF(oldLine.p1(), oldLine.p2()).normalized();
    qreal xScale = newRect.width() / oldRect.width(); // m11
    qreal yScale = newRect.height() / oldRect.height(); // m21

    if (!qIsFinite(xScale) || qFuzzyIsNull(xScale)) {
        xScale = 1.0;
    }
    if (!qIsFinite(yScale) || qFuzzyIsNull(yScale)) {
        yScale = 1.0;
    }

    QLineF newLine = oldLine;
    const qreal oldRectX = oldRect.x();
    const qreal oldRectY = oldRect.y();
    const qreal newRectX = newRect.x();
    const qreal newRectY = newRect.y();
    const auto p1 = newLine.p1();
    const auto p2 = newLine.p2();

    newLine.setP1({
        mapValue(xScale, p1.x(), oldRectX, newRectX),
        mapValue(yScale, p1.y(), oldRectY, newRectY)
    });
    newLine.setP2({
        mapValue(xScale, p2.x(), oldRectX, newRectX),
        mapValue(yScale, p2.y(), oldRectY, newRectY)
    });

    return newLine;
}

EditAction::EditAction(AnnotationTool *tool)
    : m_type(tool->type())
    , m_strokeWidth(tool->strokeWidth())
    , m_strokeColor(tool->strokeColor())
    , m_fillColor(tool->fillColor())
    , m_font(tool->font())
    , m_fontColor(tool->fontColor())
    , m_number(tool->number())
{
}

EditAction::EditAction(EditAction *action)
    : m_type(action->type())
    , m_strokeWidth(action->strokeWidth())
    , m_strokeColor(action->strokeColor())
    , m_fillColor(action->fillColor())
    , m_font(action->font())
    , m_fontColor(action->fontColor())
    , m_number(action->number())
    , m_lastUpdateArea(action->lastUpdateArea())
    , m_hasShadow(action->hasShadow())
{
}

EditAction::~EditAction()
{
    if (m_replacedBy && m_replacedBy->m_replaces == this) {
        m_replacedBy->m_replaces = nullptr;
    }
    if (m_replaces && m_replaces->m_replacedBy == this) {
        m_replaces->m_replacedBy = nullptr;
    }
}

AnnotationDocument::EditActionType EditAction::type() const
{
    return m_type;
}

int EditAction::strokeWidth() const
{
    return m_strokeWidth;
}

void EditAction::setStrokeWidth(int width)
{
    switch (m_type) {
    case AnnotationDocument::FreeHand:
    case AnnotationDocument::Line:
    case AnnotationDocument::Arrow:
    case AnnotationDocument::Rectangle:
    case AnnotationDocument::Ellipse: {
        auto oldUpdateRect = getUpdateArea();
        m_strokeWidth = width;
        m_lastUpdateArea = oldUpdateRect.united(getUpdateArea());
        break;
    }
    default:
        break;
    }
}

QColor EditAction::strokeColor() const
{
    return m_strokeColor;
}

void EditAction::setStrokeColor(const QColor &color)
{
    switch (m_type) {
    case AnnotationDocument::FreeHand:
    case AnnotationDocument::Line:
    case AnnotationDocument::Arrow:
    case AnnotationDocument::Rectangle:
    case AnnotationDocument::Ellipse:
        m_strokeColor = color;
        m_lastUpdateArea = getUpdateArea();
        break;
    default:
        break;
    }
}

QColor EditAction::fillColor() const
{
    return m_fillColor;
}

void EditAction::setFillColor(const QColor &color)
{
    switch (m_type) {
    case AnnotationDocument::Rectangle:
    case AnnotationDocument::Ellipse:
    case AnnotationDocument::Number:
        m_fillColor = color;
        m_lastUpdateArea = getUpdateArea();
        break;
    default:
        break;
    }
}

QFont EditAction::font() const
{
    return m_font;
}

void EditAction::setFont(const QFont &font)
{
    switch (m_type) {
    case AnnotationDocument::Text:
    case AnnotationDocument::Number: {
        auto oldUpdateRect = getUpdateArea();
        m_font = font;
        m_lastUpdateArea = oldUpdateRect.united(getUpdateArea());
        break;
    }
    default:
        break;
    }
}

QColor EditAction::fontColor() const
{
    return m_fontColor;
}

void EditAction::setFontColor(const QColor &color)
{
    switch (m_type) {
    case AnnotationDocument::Text:
    case AnnotationDocument::Number:
        m_fontColor = color;
        m_lastUpdateArea = getUpdateArea();
        break;
    default:
        break;
    }
}

int EditAction::number() const
{
    return m_number;
}

void EditAction::setNumber(int number)
{
    if (m_type == AnnotationDocument::Number) {
        m_number = number;
        m_lastUpdateArea = getUpdateArea();
    }
}

QMarginsF EditAction::strokeMargins() const
{
    qreal penMargin = m_strokeWidth;
    using Type = AnnotationDocument::EditActionType;
    bool isStrokedShape = m_type == Type::Rectangle || m_type == Type::Ellipse;
    if (!isStrokedShape && (penMargin == 0 || penMargin == 1)) {
        penMargin = 1.0001;
    }
    penMargin /= 2;
    return QMarginsF(penMargin, penMargin, penMargin, penMargin);
}

QMarginsF EditAction::shadowMargins() const
{
    if (!supportsShadow()) {
        return QMarginsF(0, 0, 0, 0);
    }

    return QMarginsF(shadowBlurRadius + 1, shadowBlurRadius + 1, shadowBlurRadius + shadowOffsetX + 1, shadowBlurRadius + shadowOffsetY + 1);
}

QRectF EditAction::lastUpdateArea() const
{
    return m_lastUpdateArea;
}

QRectF EditAction::getUpdateArea() const
{
    switch (m_type) {
    case AnnotationDocument::FreeHand:
    case AnnotationDocument::Line:
    case AnnotationDocument::Arrow:
    case AnnotationDocument::Rectangle:
    case AnnotationDocument::Ellipse:
        // adjust update area to prevent clipping stroke sometimes
        return visualGeometry().adjusted(0, 0, 1, 1) + shadowMargins();
    default:
        return visualGeometry() + shadowMargins();
    }
}

bool EditAction::isVisible() const
{
    const auto &rect = visualGeometry();
    return !qFuzzyIsNull(rect.width()) && !qFuzzyIsNull(rect.height())
        && m_type != AnnotationDocument::None;
}

EditAction *EditAction::replacedBy() const
{
    return m_replacedBy;
}

EditAction *EditAction::replaces() const
{
    return m_replaces;
}

void EditAction::setReplaces(EditAction *action)
{
    if (action && action->replaces() == this) {
        return;
    }
    Q_ASSERT(m_type == action->type());
    m_replaces = action;
    action->m_replacedBy = this;
    if (m_type == AnnotationDocument::Blur || m_type == AnnotationDocument::Pixelate) {
        auto *ba1 = static_cast<ShapeAction *>(this);
        auto *ba2 = static_cast<ShapeAction *>(action);
        ba1->invalidateCache();
        ba2->invalidateCache();
    }
}

EditAction *EditAction::createCopy()
{
    switch (m_type) {
    case AnnotationDocument::FreeHand:
    case AnnotationDocument::Highlight:
        return new FreeHandAction(static_cast<FreeHandAction *>(this));
    case AnnotationDocument::Line:
    case AnnotationDocument::Arrow:
        return new LineAction(static_cast<LineAction *>(this));
    case AnnotationDocument::Rectangle:
    case AnnotationDocument::Ellipse:
    case AnnotationDocument::Blur:
    case AnnotationDocument::Pixelate:
        static_cast<ShapeAction *>(this)->invalidateCache();
        return new ShapeAction(static_cast<ShapeAction *>(this));
    case AnnotationDocument::Text:
        return new TextAction(static_cast<TextAction *>(this));
    case AnnotationDocument::Number:
        return new NumberAction(static_cast<NumberAction *>(this));
    default:
        Q_ASSERT(false);
        return nullptr;
    }
}

EditAction *EditAction::createReplacement()
{
    auto copy = createCopy();
    if (copy) {
        m_replacedBy = copy;
        copy->m_replaces = this;
    }
    return copy;
}

void EditAction::copyFrom(EditAction *other)
{
    if (!other || m_type != other->m_type) {
        return;
    }
    m_strokeWidth = other->m_strokeWidth;
    m_strokeColor = other->m_strokeColor;
    m_fillColor = other->m_fillColor;
    m_font = other->m_font;
    m_fontColor = other->m_fontColor;
    m_number = other->m_number;
    m_lastUpdateArea = other->m_lastUpdateArea;
    switch (m_type) {
    case AnnotationDocument::FreeHand:
    case AnnotationDocument::Highlight: {
        auto a = static_cast<FreeHandAction *>(this);
        auto o = static_cast<FreeHandAction *>(other);
        a->m_path = o->m_path;
        break;
    }
    case AnnotationDocument::Line:
    case AnnotationDocument::Arrow: {
        auto a = static_cast<LineAction *>(this);
        auto o = static_cast<LineAction *>(other);
        a->m_line = o->m_line;
        break;
    }
    case AnnotationDocument::Rectangle:
    case AnnotationDocument::Ellipse:
    case AnnotationDocument::Blur:
    case AnnotationDocument::Pixelate: {
        auto a = static_cast<ShapeAction *>(this);
        auto o = static_cast<ShapeAction *>(other);
        a->m_rect = o->m_rect;
        break;
    }
    case AnnotationDocument::Text: {
        auto a = static_cast<TextAction *>(this);
        auto o = static_cast<TextAction *>(other);
        a->m_boundingRect = o->m_boundingRect;
        a->m_text = o->m_text;
        break;
    }
    case AnnotationDocument::Number: {
        auto a = static_cast<NumberAction *>(this);
        auto o = static_cast<NumberAction *>(other);
        a->m_boundingRect = o->m_boundingRect;
        a->m_padding = o->m_padding;
        break;
    }
    default: break;
    }
}

void EditAction::translate(const QPointF &delta)
{
    setGeometry(geometry().translated(delta));
}

bool EditAction::supportsShadow() const
{
    return m_supportsShadow;
}

bool EditAction::hasShadow() const
{
    return m_hasShadow && m_supportsShadow;
}

void EditAction::setShadow(bool shadow)
{
    if (!m_supportsShadow || m_hasShadow == shadow) {
        return;
    }

    m_hasShadow = shadow;
    m_lastUpdateArea = getUpdateArea();
}

QRectF EditAction::visualGeometry() const
{
    return geometry().normalized();
}

void EditAction::setVisualGeometry(const QRectF &geom)
{
    setGeometry(geom);
}

bool EditAction::operator==(const EditAction *other) const
{
    if (!other) {
        return false;
    }
    bool isEqual = m_type == other->m_type
                && m_lastUpdateArea == other->m_lastUpdateArea;
    switch (m_type) {
    case AnnotationDocument::FreeHand:
    case AnnotationDocument::Highlight: {
        auto a = static_cast<const FreeHandAction *>(this);
        auto o = static_cast<const FreeHandAction *>(other);
        isEqual &= a->m_strokeWidth == o->m_strokeWidth
                && a->m_strokeColor == o->m_strokeColor
                && a->m_path == o->m_path;
        break;
    }
    case AnnotationDocument::Line:
    case AnnotationDocument::Arrow: {
        auto a = static_cast<const LineAction *>(this);
        auto o = static_cast<const LineAction *>(other);
        isEqual &= a->m_strokeWidth == o->m_strokeWidth
                && a->m_strokeColor == o->m_strokeColor
                && a->m_line == o->m_line;
        break;
    }
    case AnnotationDocument::Rectangle:
    case AnnotationDocument::Ellipse: {
        auto a = static_cast<const ShapeAction *>(this);
        auto o = static_cast<const ShapeAction *>(other);
        isEqual &= a->m_rect == o->m_rect
                && a->m_strokeWidth == o->m_strokeWidth
                && a->m_strokeColor == o->m_strokeColor
                && a->m_fillColor == o->m_fillColor;
        break;
    }
    case AnnotationDocument::Blur:
    case AnnotationDocument::Pixelate: {
        auto a = static_cast<const ShapeAction *>(this);
        auto o = static_cast<const ShapeAction *>(other);
        isEqual &= a->m_rect == o->m_rect;
        break;
    }
    case AnnotationDocument::Text: {
        auto a = static_cast<const TextAction *>(this);
        auto o = static_cast<const TextAction *>(other);
        isEqual &= a->m_boundingRect == o->m_boundingRect
                && a->m_text == o->m_text
                && a->m_font == o->m_font
                && a->m_fontColor == o->m_fontColor;
        break;
    }
    case AnnotationDocument::Number: {
        auto a = static_cast<const NumberAction *>(this);
        auto o = static_cast<const NumberAction *>(other);
        isEqual &= a->m_boundingRect == o->m_boundingRect
                && a->m_padding == o->m_padding
                && a->m_number == o->m_number
                && a->m_fillColor == o->m_fillColor
                && a->m_font == o->m_font
                && a->m_fontColor == o->m_fontColor;
        break;
    }
    default: break;
    }
    return isEqual;
}

bool EditAction::operator!=(const EditAction *other) const
{
    return !(*this == other);
}

QDebug operator<<(QDebug debug, const EditAction *action)
{
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << "EditAction(";
    if (!action) {
        return debug << "0x0)";
    }
    debug << (const void *)action;
    debug << ", type=" << action->type();
    debug << ", isValid=" << action->isValid();
    debug << ", visualGeometry=" << action->visualGeometry();
    debug << ", lastUpdateArea=" << action->lastUpdateArea();
    // print as hex number to avoid a potential loop or massive cascade of debug outputs
    debug << ", replaces=" << (const void *)action->replaces();
    debug << ", replacedBy=" << (const void *)action->replacedBy();
    debug << ')';
    return debug;
}

/////////////////////////////

DeleteAction::DeleteAction(EditAction *replaces)
    : EditAction(replaces)
{
    setReplaces(replaces);
}

DeleteAction::~DeleteAction()
{
}

bool DeleteAction::isValid() const
{
    return true;
}

void DeleteAction::setGeometry(const QRectF &geom)
{
    Q_UNUSED(geom)
}

QRectF DeleteAction::geometry() const
{
    return QRectF();
}

/////////////////////////////

FreeHandAction::FreeHandAction(AnnotationTool *tool, const QPointF &startPoint)
    : EditAction(tool)
    , m_path(startPoint)
{
    m_supportsShadow = m_type == AnnotationDocument::FreeHand;
    if (m_type != AnnotationDocument::FreeHand && m_type != AnnotationDocument::Highlight) {
        m_type = AnnotationDocument::FreeHand;
    }
    m_lastUpdateArea = getUpdateArea();
}

FreeHandAction::FreeHandAction(FreeHandAction *action)
    : EditAction(action)
    , m_path(action->path())
{
    m_supportsShadow = m_type == AnnotationDocument::FreeHand;
}

FreeHandAction::~FreeHandAction()
{
}

QPainterPath FreeHandAction::path() const
{
    return m_path;
}

void FreeHandAction::addPoint(const QPointF &point)
{
    if (m_path.currentPosition() == point) {
        return;
    }
    m_path.lineTo(point);
    m_lastUpdateArea = getUpdateArea();
}

bool FreeHandAction::isValid() const
{
    return true;
}

void FreeHandAction::makeSmooth()
{
    QPainterPath smoothPath(m_path.elementAt(0));
    for (int i = 0; i < m_path.elementCount() - 1; ++i) {
        const QPointF element = m_path.elementAt(i);
        const QPointF nextElement = m_path.elementAt(i + 1);
        smoothPath.quadTo(element, (element + nextElement) / 2);
    }
    smoothPath.lineTo(m_path.currentPosition());
    m_path = smoothPath;
}

QRectF FreeHandAction::geometry() const
{
    // controlPointRect() and boundingRect() return QRectF() if the first moveTo is the only element
    return m_path.isEmpty() ? QRectF(m_path.elementAt(0), QSizeF()) : m_path.controlPointRect();
}

void FreeHandAction::setGeometry(const QRectF &geom)
{
    auto oldRect = geometry();
    auto newNormalized = geom.normalized();
    if (oldRect == newNormalized
        || (qFuzzyIsNull(newNormalized.width())
            && qFuzzyIsNull(newNormalized.height()))) {
        return;
    }
    auto updateRect = getUpdateArea();
    // TODO: deal with negative geometry instead of normalizing
    m_path = mapPathToRect(newNormalized, m_path);
    m_lastUpdateArea = updateRect.united(getUpdateArea());
}

QRectF FreeHandAction::visualGeometry() const
{
    return geometry() + strokeMargins();
}

void FreeHandAction::setVisualGeometry(const QRectF &geom)
{
    setGeometry(visualMarginsAdded(geom, -strokeMargins()));
}

/////////////////////////////

LineAction::LineAction(AnnotationTool *tool, const QPointF &startPoint)
    : EditAction(tool)
    , m_line(startPoint, startPoint)
{
    m_supportsShadow = true;

    if (m_type != AnnotationDocument::Line && m_type != AnnotationDocument::Arrow) {
        m_type = AnnotationDocument::Line;
    }
}

LineAction::LineAction(LineAction *action)
    : EditAction(action)
    , m_line(action->line())
{
    m_supportsShadow = true;
}

LineAction::~LineAction()
{
}

QLineF LineAction::line() const
{
    return m_line;
}

void LineAction::setEndPoint(const QPointF &endPoint)
{
    if (m_line.p2() == endPoint) {
        return;
    }

    auto updateRect = getUpdateArea();
    auto oldLine = m_line;
    m_line.setP2(endPoint);
    if (m_type == AnnotationDocument::Arrow) {
        auto oldArrowHeadRect = arrowHeadPolygon(oldLine).boundingRect() + strokeMargins();
        auto newArrowHeadRect = arrowHeadPolygon(m_line).boundingRect() + strokeMargins();
        updateRect = updateRect.united(oldArrowHeadRect).united(newArrowHeadRect);
    }
    m_lastUpdateArea = updateRect.united(getUpdateArea());
}

bool LineAction::isValid() const
{
    return !m_line.isNull();
}

void LineAction::setGeometry(const QRectF &geom)
{
    if (geometry() == geom) {
        return;
    }
    auto updateRect = getUpdateArea();
    auto oldLine = m_line;
    // TODO: deal with negative geometry instead of normalizing
    m_line = mapLineToRect(geom.normalized(), m_line);
    if (m_type == AnnotationDocument::Arrow) {
        auto oldArrowHeadRect = arrowHeadPolygon(oldLine).boundingRect() + strokeMargins();
        auto newArrowHeadRect = arrowHeadPolygon(m_line).boundingRect() + strokeMargins();
        updateRect = updateRect.united(oldArrowHeadRect).united(newArrowHeadRect);
    }
    m_lastUpdateArea = updateRect.united(getUpdateArea());
}

QRectF LineAction::geometry() const
{
    return {m_line.p1(), m_line.p2()};
}

QRectF LineAction::visualGeometry() const
{
    return geometry().normalized() + strokeMargins();
}

void LineAction::setVisualGeometry(const QRectF &geom)
{
    setGeometry(visualMarginsAdded(geom, -strokeMargins()));
}

QPolygonF LineAction::arrowHeadPolygon(const QLineF &mainLine) const
{
    const auto &end = mainLine.p2();
    const qreal length = qMin(mainLine.length(), 10.0);
    const qreal angle = mainLine.angle() + 180;
    auto headLine1 = QLineF::fromPolar(length, angle + 30).translated(end);
    auto headLine2 = QLineF::fromPolar(length, angle - 30).translated(end);
    return QVector<QPointF>{headLine1.p2(), end, headLine2.p2()};
}

/////////////////////////////

ShapeAction::ShapeAction(AnnotationTool *tool, const QPointF &startPoint)
    : EditAction(tool)
    , m_rect(startPoint, QSizeF(0, 0))
{
    m_supportsShadow = m_type != AnnotationDocument::Blur && m_type != AnnotationDocument::Pixelate;

    if (m_type != AnnotationDocument::Rectangle
        && m_type != AnnotationDocument::Ellipse
        && m_type != AnnotationDocument::Blur
        && m_type != AnnotationDocument::Pixelate) {
        m_type = AnnotationDocument::Rectangle;
    }
}

ShapeAction::ShapeAction(ShapeAction *action)
    : EditAction(action)
    , m_rect(action->geometry())
{
    m_supportsShadow = m_type != AnnotationDocument::Blur && m_type != AnnotationDocument::Pixelate;
    action->invalidateCache();
}

ShapeAction::~ShapeAction()
{
}

bool ShapeAction::isValid() const
{
    return !qFuzzyIsNull(m_rect.width()) && !qFuzzyIsNull(m_rect.height());
}

void ShapeAction::setGeometry(const QRectF &geom)
{
    if (m_rect == geom) {
        return;
    }
    auto updateRect = getUpdateArea();
    m_rect = geom;
    m_lastUpdateArea = updateRect.united(getUpdateArea());
}

QRectF ShapeAction::geometry() const
{
    return m_rect;
}

QRectF ShapeAction::visualGeometry() const
{
    return geometry().normalized() + strokeMargins();
}

void ShapeAction::setVisualGeometry(const QRectF &geom)
{
    setGeometry(visualMarginsAdded(geom, -strokeMargins()));
}

QImage &ShapeAction::backingStoreCache()
{
    return m_backingStoreCache;
}

void ShapeAction::invalidateCache()
{
    m_backingStoreCache = QImage();
}

/////////////////////////////

TextAction::TextAction(AnnotationTool *tool, const QPointF &startPoint)
    : EditAction(tool)
{
    m_supportsShadow = true;
    QFontMetricsF m(font());
    m_boundingRect = {startPoint.x(), startPoint.y() - m.height() / 2, 0, m.height()};
}

TextAction::TextAction(TextAction *action)
    : EditAction(action)
    , m_boundingRect(action->geometry())
    , m_text(action->text())
{
    m_supportsShadow = true;
}

TextAction::~TextAction()
{
}

QPointF TextAction::startPoint() const
{
    return m_boundingRect.topLeft();
}

QString TextAction::text() const
{
    return m_text;
}

void TextAction::setText(const QString &text)
{
    m_text = text;
    QFontMetricsF m(font());
    auto oldRect = m_boundingRect;
    m_boundingRect = m.boundingRect(text);
    m_boundingRect.setWidth(qMax(m_boundingRect.width(), m.horizontalAdvance(text)));
    m_boundingRect.moveTopLeft(oldRect.topLeft());
    m_lastUpdateArea = m_boundingRect.united(oldRect) + shadowMargins();
}

bool TextAction::isValid() const
{
    return !m_boundingRect.isEmpty();
}

void TextAction::setGeometry(const QRectF &geom)
{
    auto oldRect = m_boundingRect;
    m_boundingRect.moveTopLeft(geom.topLeft());
    m_lastUpdateArea = m_boundingRect.united(oldRect) + shadowMargins();
}

QRectF TextAction::geometry() const
{
    return m_boundingRect;
}

/////////////////////////////

NumberAction::NumberAction(AnnotationTool *tool, const QPointF &startPoint)
    : EditAction(tool)
{
    m_supportsShadow = true;
    QFontMetricsF m(font());
    m_boundingRect = m.boundingRect(QString::number(tool->number()));
    const qreal size = qMax(m_boundingRect.width(), m_boundingRect.height()) + m_padding * 2;
    m_boundingRect.setSize(QSizeF(size, size));
    m_boundingRect.translate(startPoint - QPointF(size / 2, 0));
    m_lastUpdateArea = m_boundingRect + shadowMargins();
}

NumberAction::NumberAction(NumberAction *action)
    : EditAction(action)
    , m_boundingRect(action->geometry())
    , m_padding(action->padding())
{
    m_supportsShadow = true;
}

NumberAction::~NumberAction()
{
}

QPointF NumberAction::startPoint() const
{
    return m_boundingRect.topLeft();
}

QRectF NumberAction::boundingRect() const
{
    return m_boundingRect;
}

int NumberAction::padding() const
{
    return m_padding;
}

bool NumberAction::isValid() const
{
    return true;
}

void NumberAction::setGeometry(const QRectF &geom)
{
    const auto oldRect = m_boundingRect;
    m_boundingRect.moveTopLeft(geom.topLeft());
    m_lastUpdateArea = oldRect.united(m_boundingRect) + shadowMargins();
}

QRectF NumberAction::geometry() const
{
    return m_boundingRect;
}

#include <moc_EditAction.cpp>
