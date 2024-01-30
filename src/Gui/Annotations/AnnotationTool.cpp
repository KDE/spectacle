/* SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "AnnotationTool.h"
#include "settings.h"

using enum AnnotationTool::Tool;
using enum AnnotationTool::Option;

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
    case HighlightTool:
        return StrokeOption;
    case FreeHandTool:
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
    case FreeHandTool:
    case LineTool:
    case ArrowTool:
        return 4;
    case HighlightTool:
        return 20;
    default:
        return 0;
    }
}

int AnnotationTool::strokeWidthForType(AnnotationTool::Tool type) const
{
    switch (type) {
    case FreeHandTool:
        return Settings::freehandStrokeWidth();
    case HighlightTool:
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
    case FreeHandTool:
        Settings::setFreehandStrokeWidth(width);
        break;
    case HighlightTool:
        Settings::setHighlighterStrokeWidth(width);
        break;
    case LineTool:
        Settings::setLineStrokeWidth(width);
        break;
    case ArrowTool:
        Settings::setArrowStrokeWidth(width);
        break;
    case RectangleTool:
        Settings::setRectangleStrokeWidth(width);
        break;
    case EllipseTool:
        Settings::setEllipseStrokeWidth(width);
        break;
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
    case FreeHandTool:
    case LineTool:
    case ArrowTool:
        return Qt::red;
    case HighlightTool:
        return Qt::yellow;
    case RectangleTool:
    case EllipseTool:
        return Qt::black;
    default:
        return Qt::transparent;
    }
}

QColor AnnotationTool::strokeColorForType(AnnotationTool::Tool type) const
{
    switch (type) {
    case FreeHandTool:
        return Settings::freehandStrokeColor();
    case HighlightTool:
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
    case FreeHandTool:
        Settings::setFreehandStrokeColor(color);
        break;
    case HighlightTool:
        Settings::setHighlighterStrokeColor(color);
        break;
    case LineTool:
        Settings::setLineStrokeColor(color);
        break;
    case ArrowTool:
        Settings::setArrowStrokeColor(color);
        break;
    case RectangleTool:
        Settings::setRectangleStrokeColor(color);
        break;
    case EllipseTool:
        Settings::setEllipseStrokeColor(color);
        break;
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
    case RectangleTool:
    case EllipseTool:
    case NumberTool:
        return Qt::red;
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
    case RectangleTool:
        Settings::setRectangleFillColor(color);
        break;
    case EllipseTool:
        Settings::setEllipseFillColor(color);
        break;
    case NumberTool:
        Settings::setNumberFillColor(color);
        break;
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
    case TextTool:
        Settings::setTextFont(font);
        break;
    case NumberTool:
        Settings::setNumberFont(font);
        break;
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
    case TextTool:
    case NumberTool:
        return Qt::black;
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
    case TextTool:
        Settings::setTextFontColor(color);
        break;
    case NumberTool:
        Settings::setNumberFontColor(color);
        break;
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
    case FreeHandTool:
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
    case FreeHandTool:
        Settings::setFreehandShadow(shadow);
        break;
    case LineTool:
        Settings::setLineShadow(shadow);
        break;
    case ArrowTool:
        Settings::setArrowShadow(shadow);
        break;
    case RectangleTool:
        Settings::setRectangleShadow(shadow);
        break;
    case EllipseTool:
        Settings::setEllipseShadow(shadow);
        break;
    case TextTool:
        Settings::setTextShadow(shadow);
        break;
    case NumberTool:
        Settings::setNumberShadow(shadow);
        break;
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
