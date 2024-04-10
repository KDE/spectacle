/* SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 * SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "Traits.h"
#include "Geometry.h"
#include "QtCV.h"
#include <QLocale>
#include <QUuid>

using namespace Qt::StringLiterals;
using G = ::Geometry;

// Stroke

QPen Traits::Stroke::defaultPen()
{
    return {Qt::NoBrush, 1.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin};
}

int Traits::Text::textFlags() const
{
    return (index() == Text::String ? Qt::AlignLeft | Qt::AlignTop : Qt::AlignCenter) //
        | Qt::TextDontClip | Qt::TextExpandTabs | Qt::TextIncludeTrailingSpaces;
}

QString Traits::Text::text() const
{
    if (index() == String) {
        return std::get<String>(*this);
    } else if (index() == Number) {
        return QLocale::system().toString(std::get<Number>(*this));
    }
    return {};
}

// ImageEffects

static const auto factorKey = u"factor"_s;

Traits::ImageEffects::Blur::Blur(uint factor)
    : factor(factor)
{
}

bool Traits::ImageEffects::Blur::isValid() const
{
    return factor >= minimum;
}

QImage Traits::ImageEffects::Blur::image(std::function<QImage()> getImage, QRectF rect, qreal dpr) const
{
    if (!isValid()) {
        return {};
    }
    if ((backingStoreCache.isNull() //
         || backingStoreCache.devicePixelRatio() != dpr //
         || backingStoreCache.text(factorKey).toFloat() != factor)
        && getImage) {
        backingStoreCache = getImage();
        if (backingStoreCache.isNull()) {
            return backingStoreCache;
        }
        auto mat = QtCV::qImageToMat(backingStoreCache);
        // Scale the factor with the devicePixelRatio.
        // This way high DPI pictures aren't visually affected less than standard DPI pictures.
        const qreal sigma = factor * dpr;
        QtCV::stackOrGaussianBlurCompatibility(mat, mat, {}, sigma, sigma);
        backingStoreCache.setDevicePixelRatio(dpr);
        backingStoreCache.setText(factorKey, QString::number(factor));
    }
    QRect copyRect = G::rectScaled(rect, backingStoreCache.devicePixelRatio()).toAlignedRect();
    if (copyRect.size() != backingStoreCache.size()) {
        return backingStoreCache.copy(copyRect);
    }
    return backingStoreCache;
}

Traits::ImageEffects::Pixelate::Pixelate(uint factor)
    : factor(factor)
{
}

bool Traits::ImageEffects::Pixelate::isValid() const
{
    return factor >= minimum;
}

QImage Traits::ImageEffects::Pixelate::image(std::function<QImage()> getImage, QRectF rect, qreal dpr) const
{
    if (!isValid()) {
        return {};
    }
    if ((backingStoreCache.isNull() //
         || backingStoreCache.devicePixelRatio() != dpr //
         || backingStoreCache.text(factorKey).toFloat() != factor)
        && getImage) {
        backingStoreCache = getImage();
        if (backingStoreCache.isNull()) {
            return backingStoreCache;
        }
        // Scale the factor with the devicePixelRatio.
        // This way high DPI pictures aren't visually affected less than standard DPI pictures.
        const auto effectFactor = factor * dpr;
        auto scaleDown = QTransform::fromScale(1 / effectFactor, 1 / effectFactor);
        auto scaleUp = QTransform::fromScale(effectFactor, effectFactor);
        // Smooth when scaling down to average out the colors.
        backingStoreCache = backingStoreCache.transformed(scaleDown, Qt::SmoothTransformation);
        backingStoreCache = backingStoreCache.transformed(scaleUp, Qt::FastTransformation);
        backingStoreCache.setDevicePixelRatio(dpr);
        backingStoreCache.setText(factorKey, QString::number(factor));
    }
    QRect copyRect = G::rectScaled(rect, backingStoreCache.devicePixelRatio()).toAlignedRect();
    if (copyRect.size() != backingStoreCache.size()) {
        return backingStoreCache.copy(copyRect);
    }
    return backingStoreCache;
}

// Functions

Traits::Translation Traits::unTranslateScale(qreal sx, qreal sy, const QPointF &oldPoint)
{
    return {-oldPoint.x() * sx + oldPoint.x(), -oldPoint.y() * sy + oldPoint.y()};
}

Traits::Scale Traits::scaleForSize(const QSizeF &oldSize, const QSizeF &newSize)
{
    // We should never divide by zero and we don't need fractional sizes less than 1.
    auto absWidth = std::abs(oldSize.width());
    auto absHeight = std::abs(oldSize.height());
    auto wSign = std::copysign(1.0, oldSize.width());
    auto hSign = std::copysign(1.0, oldSize.height());
    // Don't allow an absolute size less than 1x1.
    const auto wDivisor = std::max(1.0, absWidth) * wSign;
    const auto hDivisor = std::max(1.0, absHeight) * hSign;
    return {newSize.width() / wDivisor, newSize.height() / hDivisor};
}

QPainterPath Traits::minPath(const QPainterPath &path)
{
    if (path.isEmpty()) {
        auto start = path.elementCount() > 0 ? path.elementAt(0) : QPainterPath::Element{};
        QPainterPath dotPath(start);
        dotPath.lineTo(start.x + 0.0001, start.y);
        return dotPath;
    }
    return path;
}

QPainterPath Traits::arrowHead(const QLineF &mainLine, qreal strokeWidth)
{
    const auto &end = mainLine.p2();
    // This should leave a decently sized gap between the arrow head and shaft
    // and a decently sized length for all stroke widths.
    // Arrow head length will grow with stroke width.
    const qreal length = qMax(8.0, strokeWidth * 3.0);
    const qreal angle = mainLine.angle() + 180;
    auto headLine1 = QLineF::fromPolar(length, angle + 30).translated(end);
    auto headLine2 = QLineF::fromPolar(length, angle - 30).translated(end);
    QPainterPath path(headLine1.p2());
    path.lineTo(end);
    path.lineTo(headLine2.p2());
    return path;
}

QPainterPath Traits::createTextPath(const OptTuple &traits)
{
    auto &geometry = std::get<Geometry::Opt>(traits);
    auto &text = std::get<Text::Opt>(traits);
    if (!geometry) {
        return {};
    }
    if (!text) {
        return geometry->path;
    }
    const auto &start = geometry->path.elementCount() > 0 ? geometry->path.elementAt(0) : QPainterPath::Element{};
    QRectF rect{start, start};
    QFontMetricsF fm(text->font);
    QPainterPath path{start};
    if (text->index() == Text::String) {
        // Same as QPainter's default
        const auto tabStopDistance = qRound(fm.horizontalAdvance(u'x') * 8);
        auto size = fm.size(text->textFlags(), text->text(), tabStopDistance);
        size.rwidth() = std::max(size.width(), fm.height());
        size.rheight() = std::max(size.height(), fm.height());
        // TODO: RTL language reversal
        rect.adjust(0, -fm.height() / 2, size.width(), size.height() - fm.height() / 2);
        path.addRect(rect);
    } else if (text->index() == Text::Number) {
        auto margin = fm.capHeight() * 1.33;
        rect.adjust(-margin, -margin, margin, margin);
        path.addEllipse(rect);
    }
    return path;
}

QPainterPath Traits::createStrokePath(const OptTuple &traits)
{
    auto &geometry = std::get<Geometry::Opt>(traits);
    auto &stroke = std::get<Stroke::Opt>(traits);
    if (!geometry && !stroke) {
        return {};
    }
    QPainterPathStroker stroker(stroke->pen);
    auto minPath = Traits::minPath(geometry->path); // Will always have at least 2 points.
    if (auto &arrow = std::get<Arrow::Opt>(traits)) {
        const int size = minPath.elementCount();
        const QLineF lastLine{minPath.elementAt(size - 2), minPath.elementAt(size - 1)};
        auto arrowHead = Traits::arrowHead(lastLine, stroke->pen.widthF());
        return stroker.createStroke(minPath) | stroker.createStroke(arrowHead);
    } else {
        return stroker.createStroke(minPath);
    }
}

QPainterPath Traits::createInteractivePath(const OptTuple &traits)
{
    auto &geometry = std::get<Geometry::Opt>(traits);
    auto &stroke = std::get<Stroke::Opt>(traits);
    QPainterPath mousePath;
    if (geometry && !geometry->path.isEmpty()) {
        mousePath = geometry->path;
    }
    // Ensure you can click anywhere within the bounds.
    mousePath.setFillRule(Qt::WindingFill);
    if (stroke && !stroke->path.isEmpty()) {
        mousePath |= stroke->path;
    }

    return mousePath.simplified();
}

QRectF Traits::createVisualRect(const OptTuple &traits)
{
    auto &geometry = std::get<Geometry::Opt>(traits);
    auto &stroke = std::get<Stroke::Opt>(traits);
    if (!geometry) {
        return {};
    }
    QRectF visualRect;
    if (stroke) {
        visualRect = stroke->path.boundingRect() | geometry->path.boundingRect();
    } else {
        visualRect = geometry->path.boundingRect();
    }
    // Add Shadow margins if not empty.
    auto &shadow = std::get<Shadow::Opt>(traits);
    if (shadow && shadow->enabled && !visualRect.isEmpty()) {
        visualRect += Shadow::margins;
    }
    return visualRect;
}

void Traits::fastInitOptTuple(OptTuple &traits)
{
    auto &geometry = std::get<Geometry::Opt>(traits);
    if (geometry) {
        // Set Geometry::path from Font and Text/Number if empty.
        auto &text = std::get<Text::Opt>(traits);
        if (geometry->path.isEmpty() && text) {
            geometry->path = Traits::createTextPath(traits);
        }
        // Set Stroke::path from Geometry and Arrow if empty.
        auto &stroke = std::get<Stroke::Opt>(traits);
        if (stroke && stroke->path.isEmpty()) {
            stroke->path = createStrokePath(traits);
        }
        // Set Visual::rect from Stroke and Geometry if empty.
        auto &visual = std::get<Visual::Opt>(traits);
        if (visual && visual->rect.isEmpty()) {
            visual->rect = createVisualRect(traits);
        }
    }
}

void Traits::initOptTuple(OptTuple &traits)
{
    fastInitOptTuple(traits);
    auto &interactive = std::get<Interactive::Opt>(traits);
    if (interactive && interactive->path.isEmpty()) {
        // Set Interactive::path from Stroke and Geometry if empty.
        interactive->path = createInteractivePath(traits);
    }
}

template<typename T>
void clearForInitHelper(Traits::OptTuple &traits)
{
    auto &traitOpt = std::get<std::optional<T>>(traits);
    if (!traitOpt) {
        return;
    }
    auto &trait = traitOpt.value();
    if constexpr (std::same_as<T, Traits::Interactive>) {
        trait.path.clear();
    } else if constexpr (std::same_as<T, Traits::Visual>) {
        trait.rect = {};
    } else if constexpr (std::same_as<T, Traits::Stroke>) {
        trait.path.clear();
    } else if constexpr (std::same_as<T, Traits::Text>) {
        auto &geometry = std::get<Traits::Geometry::Opt>(traits);
        if (!geometry) {
            return;
        }
        if (trait.index() == Traits::Text::String) {
            QFontMetricsF fm(trait.font);
            // TODO: RTL language reversal
            QPointF topLeft;
            if (geometry->path.elementCount() == 1) {
                topLeft = geometry->path.elementAt(0);
            } else {
                topLeft = geometry->path.boundingRect().topLeft();
            }
            geometry->path = QPainterPath{topLeft + QPointF{0, fm.height() / 2}};
        } else if (trait.index() == Traits::Text::Number) {
            QPointF point;
            if (geometry->path.elementCount() == 1) {
                point = geometry->path.elementAt(0);
            } else {
                point = geometry->path.boundingRect().center();
            }
            geometry->path = QPainterPath{point};
        }
    }
}

void Traits::clearForInit(OptTuple &traits)
{
    clearForInitHelper<Interactive>(traits);
    clearForInitHelper<Visual>(traits);
    clearForInitHelper<Stroke>(traits);
    clearForInitHelper<Text>(traits);
}

void Traits::reInitTraits(OptTuple &traits)
{
    clearForInit(traits);
    initOptTuple(traits);
}

void Traits::transformTraits(const QTransform &transform, OptTuple &traits)
{
    if (transform.isIdentity()) {
        return;
    }
    auto &geometry = std::get<Geometry::Opt>(traits);
    auto &text = std::get<Text::Opt>(traits);
    bool onlyTranslating = transform.type() == QTransform::TxTranslate || text;
    if (geometry && onlyTranslating) {
        geometry->path.translate(transform.dx(), transform.dy());
    } else if (geometry) {
        geometry->path = transform.map(geometry->path);
    }
    auto &interactive = std::get<Interactive::Opt>(traits);
    if (interactive && onlyTranslating) {
        interactive->path.translate(transform.dx(), transform.dy());
    } else if (interactive) {
        interactive->path = transform.map(interactive->path);
    }
    auto &visual = std::get<Visual::Opt>(traits);
    if (visual && onlyTranslating) {
        // This is dependent on other traits, but as long as all traits have,
        // the same transformations, transforming at this time should be fine.
        visual->rect.translate(transform.dx(), transform.dy());
    } else if (geometry) {
        // This is dependent on other traits, but as long as all traits have,
        // the same transformations, transforming at this time should be fine.
        visual->rect = transform.mapRect(visual->rect);
    }
    auto &stroke = std::get<Stroke::Opt>(traits);
    if (stroke && onlyTranslating) {
        // If the stroke already has the arrow in it,
        // we shouldn't need to completely regenerate the stroke with QPainterPathStroker.
        stroke->path.translate(transform.dx(), transform.dy());
    } else if (stroke) {
        stroke->path = transform.map(stroke->path);
    }
}

// Whether the values of the traits without std::optional are considered valid.
template<>
bool Traits::isValidTrait<Traits::Geometry>(const Traits::Geometry &trait)
{
    return !trait.path.isEmpty();
}
template<>
bool Traits::isValidTrait<Traits::Interactive>(const Traits::Interactive &trait)
{
    return !trait.path.isEmpty();
}
template<>
bool Traits::isValidTrait<Traits::Visual>(const Traits::Visual &trait)
{
    return !trait.rect.isEmpty();
}
template<>
bool Traits::isValidTrait<Traits::Stroke>(const Traits::Stroke &trait)
{
    return !trait.path.isEmpty();
}
template<>
bool Traits::isValidTrait<Traits::Fill>(const Traits::Fill &trait)
{
    switch (trait.index()) {
    case Fill::Brush:
        return std::get<Fill::Brush>(trait) != Qt::NoBrush;
    case Fill::Blur:
        return std::get<Fill::Blur>(trait).isValid();
    case Fill::Pixelate:
        return std::get<Fill::Pixelate>(trait).isValid();
    default:
        return false;
    }
}
template<>
bool Traits::isValidTrait<Traits::Highlight>(const Traits::Highlight &)
{
    return true;
}
template<>
bool Traits::isValidTrait<Traits::Arrow>(const Traits::Arrow &)
{
    return true;
}
template<>
bool Traits::isValidTrait<Traits::Text>(const Traits::Text &trait)
{
    return trait.brush != Qt::NoBrush //
        && (trait.index() == Traits::Text::Number || !trait.text().isEmpty());
}
template<>
bool Traits::isValidTrait<Traits::Shadow>(const Traits::Shadow &)
{
    return true;
}
template<>
bool Traits::isValidTrait<Traits::Meta::Delete>(const Traits::Meta::Delete &)
{
    return true;
}
template<>
bool Traits::isValidTrait<Traits::Meta::Crop>(const Traits::Meta::Crop &)
{
    return true;
}

// Whether the std::optionals are considered valid.
template<typename T>
bool Traits::isValidTraitOpt(const Traits::OptTuple &traits, bool isNullValid)
{
    auto &traitOpt = std::get<std::optional<T>>(traits);
    if (!traitOpt) {
        return isNullValid;
    }
    auto &trait = traitOpt.value();

    if constexpr (std::same_as<T, Traits::Geometry>) {
        return Traits::isValidTrait(trait);
    }
    if constexpr (std::same_as<T, Traits::Interactive>) {
        return Traits::isValidTrait(trait);
    }
    if constexpr (std::same_as<T, Traits::Visual>) {
        return Traits::isValidTrait(trait);
    }
    if constexpr (std::same_as<T, Meta::Delete>) {
        return Traits::isValidTrait(trait);
    }

    // Traits that depend on geometry
    auto &geometry = std::get<Traits::Geometry::Opt>(traits);
    const bool validGeometry = geometry && Traits::isValidTrait(geometry.value());
    if constexpr (std::same_as<T, Stroke>) {
        return validGeometry && Traits::isValidTrait(trait);
    }
    if constexpr (std::same_as<T, Fill>) {
        return validGeometry && Traits::isValidTrait(trait);
    }
    if constexpr (std::same_as<T, Text>) {
        return validGeometry && Traits::isValidTrait(trait);
    }
    if constexpr (std::same_as<T, Meta::Crop>) {
        return validGeometry && Traits::isValidTrait(trait);
    }

    // Traits that depend on vector graphic traits
    auto &stroke = std::get<Stroke::Opt>(traits);
    auto &fill = std::get<Fill::Opt>(traits);
    auto &text = std::get<Text::Opt>(traits);
    const bool validStroke = stroke && Traits::isValidTrait(stroke.value());
    const bool validFill = fill && Traits::isValidTrait(fill.value());
    const bool validText = text && Traits::isValidTrait(text.value());
    if constexpr (std::same_as<T, Highlight>) {
        return validGeometry && (validStroke || validFill || validText) //
            && Traits::isValidTrait(trait);
    }
    if constexpr (std::same_as<T, Arrow>) {
        return validGeometry && (validStroke || validFill || validText) //
            && Traits::isValidTrait(trait);
    }
    if constexpr (std::same_as<T, Shadow>) {
        return validGeometry && (validStroke || validFill || validText) //
            && Traits::isValidTrait(trait);
    }
    return false;
}

template<typename... Ts>
bool isValidHelper(const Traits::OptTuple &traits)
{
    return (Traits::isValidTraitOpt<Ts>(traits, true) && ...);
}

bool Traits::isValid(const OptTuple &traits)
{
    return isValidHelper<Geometry, Interactive, Visual, Stroke, Fill, Highlight, Arrow, Text, Shadow, Meta::Delete, Meta::Crop>(traits);
}

bool Traits::isVisible(const OptTuple &traits)
{
    return Traits::isValidTraitOpt<Visual>(traits, false) //
        && Traits::isValidTraitOpt<Geometry>(traits, false) //
        && (Traits::isValidTraitOpt<Stroke>(traits, false) //
            || Traits::isValidTraitOpt<Fill>(traits, false) //
            || Traits::isValidTraitOpt<Text>(traits, false));
}

bool Traits::canBeVisible(const OptTuple &traits)
{
    return std::get<Visual::Opt>(traits) //
        && std::get<Geometry::Opt>(traits) //
        && (std::get<Stroke::Opt>(traits) //
            || std::get<Fill::Opt>(traits) //
            || std::get<Text::Opt>(traits));
}

QPainterPath Traits::geometryPath(const OptTuple &traits)
{
    auto &geometry = std::get<Geometry::Opt>(traits);
    return geometry ? geometry->path : QPainterPath{};
}

QRectF Traits::geometryPathBounds(const OptTuple &traits)
{
    auto &geometry = std::get<Geometry::Opt>(traits);
    return geometry ? geometry->path.boundingRect() : QRectF{};
}

QPainterPath Traits::interactivePath(const OptTuple &traits)
{
    auto &interactive = std::get<Interactive::Opt>(traits);
    return interactive ? interactive->path : QPainterPath{};
}

QRectF Traits::visualRect(const OptTuple &traits)
{
    auto &visual = std::get<Visual::Opt>(traits);
    return visual ? visual->rect : QRectF{};
}

// QDebug operator<< declarations

// Traits

QDebug operator<<(QDebug debug, const Traits::Geometry &trait)
{
    using namespace Traits;
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << "Geometry" << '(';
    debug << (const void *)&trait;
    debug << ",\n    path=" << trait.path;
    debug << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const Traits::Interactive &trait)
{
    using namespace Traits;
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << "Interactive" << '(';
    debug << (const void *)&trait;
    debug << ",\n    path=" << trait.path;
    debug << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const Traits::Visual &trait)
{
    using namespace Traits;
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << "Visual" << '(';
    debug << (const void *)&trait;
    debug << ",\n    rect=" << trait.rect;
    debug << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const Traits::Stroke &trait)
{
    using namespace Traits;
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << "Stroke" << '(';
    debug << (const void *)&trait;
    debug << ",\n    pen=" << trait.pen;
    debug << ",\n    path=" << trait.path;
    debug << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const Traits::Fill &trait)
{
    using namespace Traits;
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << "Fill" << '(';
    debug << (const void *)&trait;
    debug << ", ";
    switch (trait.index()) {
    case Fill::Brush:
        debug << std::get<Fill::Brush>(trait);
        break;
    case Fill::Blur:
        debug << std::get<Fill::Blur>(trait);
        break;
    case Fill::Pixelate:
        debug << std::get<Fill::Pixelate>(trait);
        break;
    default:
        break;
    }
    debug << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const Traits::Highlight &trait)
{
    using namespace Traits;
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << "Highlight" << '(';
    debug << (const void *)&trait;
    debug << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const Traits::Arrow &trait)
{
    using namespace Traits;
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << "Arrow" << '(';
    debug << (const void *)&trait;
    debug << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const Traits::Text &trait)
{
    using namespace Traits;
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << "Text" << '(';
    debug << (const void *)&trait;
    debug << ",\n    text=" << trait.text();
    debug << ",\n    brush=" << trait.brush;
    debug << ",\n    font=" << trait.font;
    debug << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const Traits::Shadow &trait)
{
    using namespace Traits;
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << "Shadow" << '(';
    debug << (const void *)&trait;
    debug << ",\n    enabled=" << trait.enabled;
    debug << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const Traits::Meta::Delete &trait)
{
    using namespace Traits;
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << "Delete" << '(';
    debug << (const void *)&trait;
    debug << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const Traits::Meta::Crop &trait)
{
    using namespace Traits;
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << "Crop" << '(';
    debug << (const void *)&trait;
    debug << ')';
    return debug;
}

// ImageEffects

QDebug operator<<(QDebug debug, const Traits::ImageEffects::Blur &ref)
{
    using namespace Traits::ImageEffects;
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << "Blur" << '(';
    debug << (const void *)&ref;
    debug << ", factor=" << ref.factor;
    debug << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const Traits::ImageEffects::Pixelate &ref)
{
    using namespace Traits::ImageEffects;
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << "Pixelate" << '(';
    debug << (const void *)&ref;
    debug << ", factor=" << ref.factor;
    debug << ')';
    return debug;
}

// Optionals
// clang-format off
#define OPTIONAL_DEBUG_DEF(ClassName)\
QDebug operator<<(QDebug debug, const Traits::ClassName::Opt &optional)\
{\
    using namespace Traits;\
    QDebugStateSaver stateSaver(debug);\
    debug.nospace();\
    debug << "Opt" << '<';\
    if (optional.has_value()) {\
        debug << optional.value();\
    } else {\
        debug << #ClassName << "(0x0)";\
    }\
    debug << ">(" << &optional << ')';\
    return debug;\
}
// clang-format on
OPTIONAL_DEBUG_DEF(Geometry)
OPTIONAL_DEBUG_DEF(Interactive)
OPTIONAL_DEBUG_DEF(Visual)
OPTIONAL_DEBUG_DEF(Stroke)
OPTIONAL_DEBUG_DEF(Fill)
OPTIONAL_DEBUG_DEF(Highlight)
OPTIONAL_DEBUG_DEF(Arrow)
OPTIONAL_DEBUG_DEF(Text)
OPTIONAL_DEBUG_DEF(Shadow)
OPTIONAL_DEBUG_DEF(Meta::Delete)
OPTIONAL_DEBUG_DEF(Meta::Crop)

#undef OPTIONAL_DEBUG_DEF

QDebug operator<<(QDebug debug, const Traits::OptTuple &optTuple)
{
    using namespace Traits;
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << "OptTuple" << '(';
    debug << (const void *)&optTuple;
    debug << ",\n  " << std::get<Traits::Geometry::Opt>(optTuple);
    debug << ",\n  " << std::get<Traits::Interactive::Opt>(optTuple);
    debug << ",\n  " << std::get<Traits::Visual::Opt>(optTuple);
    debug << ",\n  " << std::get<Traits::Stroke::Opt>(optTuple);
    debug << ",\n  " << std::get<Traits::Fill::Opt>(optTuple);
    debug << ",\n  " << std::get<Traits::Highlight::Opt>(optTuple);
    debug << ",\n  " << std::get<Traits::Arrow::Opt>(optTuple);
    debug << ",\n  " << std::get<Traits::Text::Opt>(optTuple);
    debug << ",\n  " << std::get<Traits::Shadow::Opt>(optTuple);
    debug << ",\n  " << std::get<Traits::Meta::Delete::Opt>(optTuple);
    debug << ",\n  " << std::get<Traits::Meta::Crop::Opt>(optTuple);
    debug << ')';
    return debug;
}
