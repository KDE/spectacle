/* SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "AnnotationTool.h"
#include "settings.h"

using enum AnnotationTool::Tool;
using enum AnnotationTool::Option;

// clang-format off

// Default value macros

#define DEFAULT_STROKE_WIDTH(ToolName) case ToolName##Tool: { return Settings::default##ToolName##StrokeWidthValue(); }

#define DEFAULT_STROKE_COLOR(ToolName) case ToolName##Tool: { return Settings::default##ToolName##StrokeColorValue(); }

#define DEFAULT_FILL_COLOR(ToolName) case ToolName##Tool: { return Settings::default##ToolName##FillColorValue(); }

#define DEFAULT_FONT(ToolName) case ToolName##Tool: { return Settings::default##ToolName##FontValue(); }

#define DEFAULT_FONT_COLOR(ToolName) case ToolName##Tool: { return Settings::default##ToolName##FontColorValue(); }

#define DEFAULT_SHADOW(ToolName) case ToolName##Tool: { return Settings::default##ToolName##ShadowValue(); }

// No getter macros because there's no way to lowercase the ToolName arg

// Setter macros

#define SET_STROKE_WIDTH(ToolName) case ToolName##Tool: { Settings::set##ToolName##StrokeWidth(width); } break;

#define SET_STROKE_COLOR(ToolName) case ToolName##Tool: { Settings::set##ToolName##StrokeColor(color); } break;

#define SET_FILL_COLOR(ToolName) case ToolName##Tool: { Settings::set##ToolName##FillColor(color); } break;

#define SET_FONT(ToolName) case ToolName##Tool: { Settings::set##ToolName##Font(font); } break;

#define SET_FONT_COLOR(ToolName) case ToolName##Tool: { Settings::set##ToolName##FontColor(color); } break;

#define SET_SHADOW(ToolName) case ToolName##Tool: { Settings::set##ToolName##Shadow(shadow); } break;

// clang-format on

AnnotationTool::AnnotationTool(QObject *parent)
    : QObject(parent)
{
}

AnnotationTool::~AnnotationTool()
{
}

AnnotationTool::Tool AnnotationTool::type() const
{
    return m_type;
}

void AnnotationTool::setType(AnnotationTool::Tool type)
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
    setType(AnnotationTool::NoTool);
}

bool AnnotationTool::isCreationTool() const
{
    return m_type != NoTool && m_type != SelectTool;
}

AnnotationTool::Options AnnotationTool::options() const
{
    return m_options;
}

constexpr AnnotationTool::Options AnnotationTool::optionsForType(AnnotationTool::Tool type)
{
    switch (type) {
    case HighlighterTool:
        return StrokeOption;
    case FreehandTool:
    case LineTool:
    case ArrowTool:
        return {StrokeOption, ShadowOption};
    case RectangleTool:
    case EllipseTool:
        return {StrokeOption, ShadowOption, FillOption};
    case TextTool:
        return {FontOption, TextOption, ShadowOption};
    case NumberTool:
        return {FillOption, ShadowOption, FontOption, NumberOption};
    default:
        return NoOptions;
    }
}

int AnnotationTool::strokeWidth() const
{
    return strokeWidthForType(m_type);
}

constexpr int AnnotationTool::defaultStrokeWidthForType(AnnotationTool::Tool type)
{
    switch (type) {
        DEFAULT_STROKE_WIDTH(Freehand)
        DEFAULT_STROKE_WIDTH(Highlighter)
        DEFAULT_STROKE_WIDTH(Line)
        DEFAULT_STROKE_WIDTH(Arrow)
        DEFAULT_STROKE_WIDTH(Rectangle)
        DEFAULT_STROKE_WIDTH(Ellipse)
    default:
        return 0;
    }
}

int AnnotationTool::strokeWidthForType(AnnotationTool::Tool type) const
{
    switch (type) {
    case FreehandTool:
        return Settings::freehandStrokeWidth();
    case HighlighterTool:
        return Settings::highlighterStrokeWidth();
    case LineTool:
        return Settings::lineStrokeWidth();
    case ArrowTool:
        return Settings::arrowStrokeWidth();
    case RectangleTool:
        return Settings::rectangleStrokeWidth();
    case EllipseTool:
        return Settings::ellipseStrokeWidth();
    default:
        return 0;
    }
}

void AnnotationTool::setStrokeWidth(int width)
{
    if (!m_options.testFlag(Option::StrokeOption) || strokeWidth() == width) {
        return;
    }

    setStrokeWidthForType(width, m_type);
    Q_EMIT strokeWidthChanged(width);
}

void AnnotationTool::setStrokeWidthForType(int width, AnnotationTool::Tool type)
{
    switch (type) {
        SET_STROKE_WIDTH(Freehand)
        SET_STROKE_WIDTH(Highlighter)
        SET_STROKE_WIDTH(Line)
        SET_STROKE_WIDTH(Arrow)
        SET_STROKE_WIDTH(Rectangle)
        SET_STROKE_WIDTH(Ellipse)
    default:
        break;
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

constexpr QColor AnnotationTool::defaultStrokeColorForType(AnnotationTool::Tool type)
{
    switch (type) {
        DEFAULT_STROKE_COLOR(Freehand)
        DEFAULT_STROKE_COLOR(Highlighter)
        DEFAULT_STROKE_COLOR(Line)
        DEFAULT_STROKE_COLOR(Arrow)
        DEFAULT_STROKE_COLOR(Rectangle)
        DEFAULT_STROKE_COLOR(Ellipse)
    default:
        return Qt::transparent;
    }
}

QColor AnnotationTool::strokeColorForType(AnnotationTool::Tool type) const
{
    switch (type) {
    case FreehandTool:
        return Settings::freehandStrokeColor();
    case HighlighterTool:
        return Settings::highlighterStrokeColor();
    case LineTool:
        return Settings::lineStrokeColor();
    case ArrowTool:
        return Settings::arrowStrokeColor();
    case RectangleTool:
        return Settings::rectangleStrokeColor();
    case EllipseTool:
        return Settings::ellipseStrokeColor();
    default:
        return Qt::transparent;
    }
}

void AnnotationTool::setStrokeColor(const QColor &color)
{
    if (!m_options.testFlag(Option::StrokeOption) || strokeColor() == color) {
        return;
    }

    setStrokeColorForType(color, m_type);
    Q_EMIT strokeColorChanged(color);
}

void AnnotationTool::setStrokeColorForType(const QColor &color, AnnotationTool::Tool type)
{
    switch (type) {
        SET_STROKE_COLOR(Freehand)
        SET_STROKE_COLOR(Highlighter)
        SET_STROKE_COLOR(Line)
        SET_STROKE_COLOR(Arrow)
        SET_STROKE_COLOR(Rectangle)
        SET_STROKE_COLOR(Ellipse)
    default:
        break;
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

constexpr QColor AnnotationTool::defaultFillColorForType(AnnotationTool::Tool type)
{
    switch (type) {
        DEFAULT_FILL_COLOR(Rectangle)
        DEFAULT_FILL_COLOR(Ellipse)
        DEFAULT_FILL_COLOR(Number)
    default:
        return Qt::transparent;
    }
}

QColor AnnotationTool::fillColorForType(AnnotationTool::Tool type) const
{
    switch (type) {
    case RectangleTool:
        return Settings::rectangleFillColor();
    case EllipseTool:
        return Settings::ellipseFillColor();
    case NumberTool:
        return Settings::numberFillColor();
    default:
        return Qt::transparent;
    }
}

void AnnotationTool::setFillColor(const QColor &color)
{
    if (!m_options.testFlag(Option::FillOption) || fillColor() == color) {
        return;
    }

    setFillColorForType(color, m_type);
    Q_EMIT fillColorChanged(color);
}

void AnnotationTool::setFillColorForType(const QColor &color, AnnotationTool::Tool type)
{
    switch (type) {
        SET_FILL_COLOR(Rectangle)
        SET_FILL_COLOR(Ellipse)
        SET_FILL_COLOR(Number)
    default:
        break;
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

QFont AnnotationTool::fontForType(AnnotationTool::Tool type) const
{
    switch (type) {
    case TextTool:
        return Settings::textFont();
    case NumberTool:
        return Settings::numberFont();
    default:
        return {};
    }
}

void AnnotationTool::setFont(const QFont &font)
{
    if (!m_options.testFlag(Option::FontOption) || this->font() == font) {
        return;
    }

    setFontForType(font, m_type);
    Q_EMIT fontChanged(font);
}

void AnnotationTool::setFontForType(const QFont &font, AnnotationTool::Tool type)
{
    switch (type) {
        SET_FONT(Text)
        SET_FONT(Number)
    default:
        break;
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

constexpr QColor AnnotationTool::defaultFontColorForType(AnnotationTool::Tool type)
{
    switch (type) {
        DEFAULT_FONT_COLOR(Text)
        DEFAULT_FONT_COLOR(Number)
    default:
        return Qt::transparent;
    }
}

QColor AnnotationTool::fontColorForType(AnnotationTool::Tool type) const
{
    switch (type) {
    case TextTool:
        return Settings::textFontColor();
    case NumberTool:
        return Settings::numberFontColor();
    default:
        return Qt::transparent;
    }
}

void AnnotationTool::setFontColor(const QColor &color)
{
    if (!m_options.testFlag(Option::FontOption) || fontColor() == color) {
        return;
    }

    setFontColorForType(color, m_type);
    Q_EMIT fontColorChanged(color);
}

void AnnotationTool::setFontColorForType(const QColor &color, AnnotationTool::Tool type)
{
    switch (type) {
        SET_FONT_COLOR(Text)
        SET_FONT_COLOR(Number)
    default:
        break;
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

bool AnnotationTool::typeHasShadow(AnnotationTool::Tool type) const
{
    switch (type) {
    case FreehandTool:
        return Settings::freehandShadow();
    case LineTool:
        return Settings::lineShadow();
    case ArrowTool:
        return Settings::arrowShadow();
    case RectangleTool:
        return Settings::rectangleShadow();
    case EllipseTool:
        return Settings::ellipseShadow();
    case TextTool:
        return Settings::textShadow();
    case NumberTool:
        return Settings::numberShadow();
    default:
        return false;
    }
}

bool AnnotationTool::hasShadow() const
{
    return typeHasShadow(m_type);
}

void AnnotationTool::setTypeHasShadow(AnnotationTool::Tool type, bool shadow)
{
    switch (type) {
        SET_SHADOW(Freehand)
        SET_SHADOW(Line)
        SET_SHADOW(Arrow)
        SET_SHADOW(Rectangle)
        SET_SHADOW(Ellipse)
        SET_SHADOW(Text)
        SET_SHADOW(Number)
    default:
        break;
    }
}

void AnnotationTool::setShadow(bool shadow)
{
    if (!m_options.testFlag(Option::ShadowOption) || hasShadow() == shadow) {
        return;
    }

    setTypeHasShadow(m_type, shadow);
    Q_EMIT shadowChanged(shadow);
}

void AnnotationTool::resetShadow()
{
    setShadow(true);
}

#include <moc_AnnotationTool.cpp>
