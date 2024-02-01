/* SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QBrush>
#include <QFont>
#include <QHash>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QUuid>

#include <optional>
#include <tuple>

namespace Traits
{
// Definitions all traits should have.
// Sometimes clang-format formats macros poorly, so I'm disabling clang-format on macros.
// clang-format off

#define COMMON_TRAIT_DEFS(ClassName)\
using Opt = std::optional<ClassName>;\
bool operator==(const ClassName &other) const = default;

// clang-format on

/**
 * Structs for each trait.
 *
 * These should all be kept pretty small and inexpensive to construct with minimal API.
 *
 * These should describe attributes of an abstract drawable object, not drawing tool object type.
 */

struct Geometry {
    COMMON_TRAIT_DEFS(Geometry)
    // The base geometry path for any item that should be drawable.
    QPainterPath path{};
    // The the path bounds used for mouse interaction.
    QPainterPath mousePath{};
    // The rendered area, including all traits.
    // Adequate for rendering the associated item,
    // but not for overwriting the graphics of previous items.
    QRectF visualRect{};
};

struct Stroke {
    COMMON_TRAIT_DEFS(Stroke)
    // The pen to use for Stroke::path.
    // Use pen.brush() with QPainter::setBrush instead of setPen when rendering Stroke::path.
    static QPen defaultPen();
    QPen pen = defaultPen();
    // Used for rendering, getting the visual bounds and mouse interaction areas.
    QPainterPath path{};
};

struct Fill {
    COMMON_TRAIT_DEFS(Fill)
    // The brush to use for filling Geometry::path
    QBrush brush{Qt::NoBrush};
};

struct Highlight {
    COMMON_TRAIT_DEFS(Highlight)
    // The QPainter::CompositionMode to use for highlight effects
    static constexpr QPainter::CompositionMode compositionMode = QPainter::CompositionMode_Darken;
};

struct Arrow {
    COMMON_TRAIT_DEFS(Arrow)
    // TODO: support different styles
    enum Type { OneHead };
};

struct Text {
    COMMON_TRAIT_DEFS(Text)
    using Variant = std::variant<std::monostate, QString, int>;
    enum Type : std::size_t { monostate = 0, String, Number };
    Type type() const;
    int textFlags() const;
    // Set Geometry::path to the bounding rect of this text.
    Variant value{};
    QBrush brush{Qt::NoBrush};
    QFont font{};
    QString text() const;
};

/* Not using QPixmap for images because I read from a Qt Company employee that QPixmaps are
 * almost always basically just QImages since Qt 5.
 *
 * https://www.reddit.com/r/QtFramework/comments/d9m17b/the_detailed_differences_between_qimage_and/f23m64t/
 *
 * nezitcle (Qt Company):
 *
 * > So it may be important to point out, but in Qt 5 QPixmap is almost always a QImage. This was
 * > important in Qt 4 when we had multiple paint engines and then QPixmap was the "native" image
 * > format for the platform you were running on. There is only one case I can think of where
 * > QPixmap is not a QImage and that is when you are using the DirectFB platform plugin (which is
 * > probably busted at this point, and definitely legacy code). There QPixmap is a bona fide native
 * > surface (DFBSurface), but asside from that there is no real reason to use QPixmap over QImage
 * > in Qt 5, aside from signaling to the user of an API that they shouldn't modify the pixel data
 * > directly.
 */

struct ImageEffect {
    COMMON_TRAIT_DEFS(ImageEffect)
    enum Type : unsigned int { Blur, Pixelate };

    ImageEffect(Type type, float factor);

    // The type of effect.
    // Consider switching to using an std::variant of effect type structs if we need
    // effects with different types of parameters.
    Type type{};
    // The factor by which original logical pixel sizes are multiplied.
    // One or less has no effect.
    // It's a float instead of a double because we don't need particularly large or small values
    // and float saves 4 bytes.
    float factor{};

    // Get an image that can be immediately used for rendering an image effect.
    // `getImage` should be the function used to generate the original image with no effects.
    // `rect` should be the section of the document you want to render over .
    // `dpr` should be the devicePixelRatio of the original image.
    QImage image(std::function<QImage()> getImage, QRectF rect, qreal dpr) const;

protected:
    // Setting as mutable means it can be mutated even when ImageEffect is const
    // or using a const member function.
    mutable QImage backingStoreCache{};
};

// Scaled to fit Geometry::visualRect
struct Shadow {
    COMMON_TRAIT_DEFS(Shadow)
    static constexpr int blurRadius = 2;
    static constexpr int xOffset = 2;
    static constexpr int yOffset = 2;
    static constexpr QMarginsF margins{
        blurRadius + 1,
        blurRadius + 1,
        blurRadius + xOffset + 1,
        blurRadius + yOffset + 1,
    };
    bool enabled = true;
};

// Group types
template<typename... Args>
struct ArgsType {
    using Tuple = std::tuple<Args...>;
    using OptTuple = std::tuple<typename Args::Opt...>;
};
struct All : ArgsType<Geometry, Stroke, Fill, Highlight, Arrow, Text, ImageEffect, Shadow> {
};
using OptTuple = All::OptTuple;

struct Translation {
    // QTransform: m31
    // QMatrix4x4: 3,0 or m41
    qreal dx = 0;
    // QTransform: m32
    // QMatrix4x4: 3,1 or m42
    qreal dy = 0;
};

struct Scale {
    // QTransform: m11
    // QMatrix4x4: 0,0 or m11
    qreal sx = 1;
    // QTransform: m22
    // QMatrix4x4: 1,1 or m22
    qreal sy = 1;
};

// Undo a translation caused by scaling.
// Scaling can also translate unless you apply an opposite translation.
// `oldPoint` should be the position for the geometry you will apply the scale to.
Translation unTranslateScale(qreal sx, qreal sy, const QPointF &oldPoint);

Scale scaleForSize(const QSizeF &oldSize, const QSizeF &newSize);

// The path, but with at least a tiny line to make it visible with a stroke when empty.
QPainterPath minPath(const QPainterPath &path);

// Get an arrow head for the given line and stroke width.
QPainterPath arrowHead(const QLineF &mainLine, qreal strokeWidth);

// Get the path based on the Text trait.
QPainterPath createTextPath(const OptTuple &traits);

// Get the stroke path based on the available traits.
QPainterPath createStrokePath(const OptTuple &traits);

// Constructs a mousePath based on the available traits.
QPainterPath createMousePath(const OptTuple &traits);

// Constructs a visualRect based on the available traits.
QRectF createVisualRect(const OptTuple &traits);

// Excludes computationally expensive parts that aren't needed for rendering
void fastInitOptTuple(OptTuple &traits);

// Initialize an OptTuple the way a HistoryItem should have it.
// Returns true if changes were done.
void initOptTuple(OptTuple &traits);

// Clear the given traits in the OptTuple for reinitialization.
void clearForInit(OptTuple &traits);

// clearForInit and initOptTuple
void reInitTraits(OptTuple &traits);

// Apply a transformation to the traits of OptTuple, when possible. Mainly for translations.
// If you want to scale a path, you typically don't want to scale the stroke width, so you should
// typically not use this for scaling. Avoids copying whole paths when only translating.
void transformTraits(const QTransform &transform, OptTuple &traits);

// Whether the traits are considered valid.
// It is valid to not have any traits.
bool isValid(const OptTuple &traits);

// Returns whether the set of traits can actually be seen by a user.
bool isVisible(const OptTuple &traits);

// Returns the Geometry::mousePath or an empty path if not available.
QPainterPath mousePath(const OptTuple &traits);

// Returns the Geometry::visualRect or an empty rect if not available.
QRectF visualRect(const OptTuple &traits);

#undef COMMON_TRAIT_DEFS

}

QDebug operator<<(QDebug debug, const Traits::Geometry &trait);
QDebug operator<<(QDebug debug, const Traits::Stroke &trait);
QDebug operator<<(QDebug debug, const Traits::Fill &trait);
QDebug operator<<(QDebug debug, const Traits::Highlight &trait);
QDebug operator<<(QDebug debug, const Traits::Arrow &trait);
QDebug operator<<(QDebug debug, const Traits::Text &trait);
QDebug operator<<(QDebug debug, const Traits::ImageEffect &trait);
QDebug operator<<(QDebug debug, const Traits::Shadow &trait);
QDebug operator<<(QDebug debug, const Traits::OptTuple &optTuple);
//clang-format off
#define OPTIONAL_DEBUG_DEF(ClassName)\
QDebug operator<<(QDebug debug, const std::optional<ClassName> &optional);
//clang-format on
OPTIONAL_DEBUG_DEF(Traits::Geometry)
OPTIONAL_DEBUG_DEF(Traits::Stroke)
OPTIONAL_DEBUG_DEF(Traits::Fill)
OPTIONAL_DEBUG_DEF(Traits::Highlight)
OPTIONAL_DEBUG_DEF(Traits::Arrow)
OPTIONAL_DEBUG_DEF(Traits::Text)
OPTIONAL_DEBUG_DEF(Traits::ImageEffect)
OPTIONAL_DEBUG_DEF(Traits::Shadow)
#undef OPTIONAL_DEBUG_DEF

