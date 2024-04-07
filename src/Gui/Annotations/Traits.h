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
};

struct Interactive {
    COMMON_TRAIT_DEFS(Interactive)
    // The the path bounds used for mouse interaction.
    QPainterPath path{};
};

struct Visual {
    COMMON_TRAIT_DEFS(Visual)
    // The rendered area, including all traits.
    // Adequate for rendering the associated item,
    // but not for overwriting the graphics of previous items.
    QRectF rect{};
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

namespace ImageEffects
{
struct Blur {
    // Standard deviation for the blur effect.
    // Values less than the minimum are invalid.
    qreal factor = 2;
    static constexpr qreal minimum = 0;
    Blur(uint factor);
    bool isValid() const;
    // Get an image that can be immediately used for rendering an image effect.
    // `getImage` should be the function used to generate the original image with no effects.
    // `rect` should be the section of the document you want to render over .
    // `dpr` should be the devicePixelRatio of the original image.
    QImage image(std::function<QImage()> getImage, QRectF rect, qreal dpr) const;
    bool operator==(const Blur &other) const = default;

private:
    // Setting as mutable means it can be mutated even when this is const
    // or using a const member function.
    mutable QImage backingStoreCache{};
};

struct Pixelate {
    // The factor by which original logical pixel sizes are multiplied.
    // Values less than the minimum are invalid.
    uint factor = 4;
    static constexpr qreal minimum = 1;
    Pixelate(uint factor);
    bool isValid() const;
    // Get an image that can be immediately used for rendering an image effect.
    // `getImage` should be the function used to generate the original image with no effects.
    // `rect` should be the section of the document you want to render over .
    // `dpr` should be the devicePixelRatio of the original image.
    QImage image(std::function<QImage()> getImage, QRectF rect, qreal dpr) const;
    bool operator==(const Pixelate &other) const = default;

private:
    mutable QImage backingStoreCache{};
};
}

struct Fill : public std::variant<QBrush, ImageEffects::Blur, ImageEffects::Pixelate> {
    COMMON_TRAIT_DEFS(Fill)
    enum Type : std::size_t { Brush, Blur, Pixelate, NPos = std::variant_npos };
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

// Set Geometry::path based on the bounding rect of this text.
struct Text : public std::variant<QString, int> {
    COMMON_TRAIT_DEFS(Text)
    enum Type : std::size_t { String, Number, NPos = std::variant_npos };
    int textFlags() const;
    // Set Geometry::path to the bounding rect of this text.
    QBrush brush{Qt::NoBrush};
    QFont font{};
    QString text() const;
};

// Scaled to fit Geometry::visualRect
struct Shadow {
    COMMON_TRAIT_DEFS(Shadow)
    // The radius from the edge of the base graphics to where the shadow ends.
    static constexpr qreal radius = 2;
    // The X axis offset of the shadow.
    static constexpr qreal xOffset = 2;
    // The Y axis offset of the shadow.
    static constexpr qreal yOffset = 2;
    // The margins to be added to the visualRect so that it contains the shadow.
    static constexpr QMarginsF margins{
        std::max(0.0, radius - xOffset),
        std::max(0.0, radius - yOffset),
        std::max(0.0, radius + xOffset),
        std::max(0.0, radius + xOffset),
    };
    bool enabled = true;
};

// Traits that represent a kind of change to the document, but don't do anything directly.
namespace Meta
{
struct Delete {
    COMMON_TRAIT_DEFS(Delete)
};
struct Crop {
    COMMON_TRAIT_DEFS(Crop)
};
}

using OptTuple = std::tuple<Geometry::Opt, Interactive::Opt, Visual::Opt, Stroke::Opt, Fill::Opt, Highlight::Opt, Arrow::Opt, Text::Opt, Shadow::Opt, Meta::Delete::Opt, Meta::Crop::Opt>;

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
QPainterPath createInteractivePath(const OptTuple &traits);

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

// Whether the values of the traits without std::optional are considered valid.
template<typename T>
bool isValidTrait(const T &trait);

// Whether the std::optionals are considered valid.
template<typename T>
bool isValidTraitOpt(const OptTuple &traits, bool isNullValid);

// Whether the traits are considered valid.
// It is valid to not have any traits.
bool isValid(const OptTuple &traits);

// Returns whether the set of traits can actually be seen by a user.
bool isVisible(const OptTuple &traits);

// Returns whether the set of traits can be made visible even if they currently aren't.
bool canBeVisible(const OptTuple &traits);

// Returns the Geometry::path or an empty path if not available.
QPainterPath geometryPath(const OptTuple &traits);

// Returns the Geometry::path::boundingRect or an empty rect if not available.
QRectF geometryPathBounds(const OptTuple &traits);

// Returns the Interactive::path or an empty path if not available.
QPainterPath interactivePath(const OptTuple &traits);

// Returns the Visual::rect or an empty rect if not available.
QRectF visualRect(const OptTuple &traits);

#undef COMMON_TRAIT_DEFS

}

// clang-format off
#define DEBUG_DEF(ClassName)\
QDebug operator<<(QDebug debug, const ClassName &ref);
// clang-format on

DEBUG_DEF(Traits::Geometry)
DEBUG_DEF(Traits::Interactive)
DEBUG_DEF(Traits::Visual)
DEBUG_DEF(Traits::Stroke)
DEBUG_DEF(Traits::Fill)
DEBUG_DEF(Traits::Highlight)
DEBUG_DEF(Traits::Arrow)
DEBUG_DEF(Traits::Text)
DEBUG_DEF(Traits::Shadow)
DEBUG_DEF(Traits::Meta::Delete)
DEBUG_DEF(Traits::Meta::Crop)

DEBUG_DEF(Traits::ImageEffects::Blur)
DEBUG_DEF(Traits::ImageEffects::Pixelate)

DEBUG_DEF(Traits::Geometry::Opt)
DEBUG_DEF(Traits::Interactive::Opt)
DEBUG_DEF(Traits::Visual::Opt)
DEBUG_DEF(Traits::Stroke::Opt)
DEBUG_DEF(Traits::Fill::Opt)
DEBUG_DEF(Traits::Highlight::Opt)
DEBUG_DEF(Traits::Arrow::Opt)
DEBUG_DEF(Traits::Text::Opt)
DEBUG_DEF(Traits::Shadow::Opt)
DEBUG_DEF(Traits::Meta::Delete::Opt)
DEBUG_DEF(Traits::Meta::Crop::Opt)

DEBUG_DEF(Traits::OptTuple)

#undef DEBUG_DEF
