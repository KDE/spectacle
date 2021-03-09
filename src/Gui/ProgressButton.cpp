/*
 * SPDX-FileCopyrightText: 2019 David Redondo <david@david-redondo.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "ProgressButton.h"

#include <QStyleOption>
#include <QStylePainter>

#include <KColorScheme>

ProgressButton::ProgressButton(QWidget* parent)
    : QToolButton{parent}
{
}

void ProgressButton::setProgress(double progress)
{
    mProgress = progress;
    repaint();
}

void ProgressButton::paintEvent(QPaintEvent* event)
{
    //Draw Button without text and icon, note the missing text and icon in options
    QStylePainter painter(this);
    QStyleOption toolbuttonOptions;
    toolbuttonOptions.initFrom(this);
     if (isDown()) {
        toolbuttonOptions.state.setFlag(QStyle::State_Sunken);
    } else {
        toolbuttonOptions.state.setFlag(QStyle::State_Raised);
    }
    painter.drawPrimitive(QStyle::PE_PanelButtonTool, toolbuttonOptions);
    auto pal = palette();
    if (!qFuzzyIsNull(mProgress)) {
        //Draw overlay
        KColorScheme::adjustForeground(pal, KColorScheme::PositiveText, QPalette::Button,  KColorScheme::Button);
        QStyleOption overlayOption;
        overlayOption.rect = layoutDirection() == Qt::LeftToRight
                                ? QRect(0, 0, width() *  mProgress, height())
                                : QRect(width() * (1-mProgress), 0, width(), height());
        overlayOption.palette = pal;
        overlayOption.state.setFlag(QStyle::State_Sunken, isDown());
        painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
        painter.setOpacity(0.5);
        painter.drawPrimitive(QStyle::PE_PanelButtonTool, overlayOption);
    }
    //Finally draw text and icon and outline
    QStyleOptionToolButton labelOptions;
    labelOptions.initFrom(this);
    labelOptions.text = text();
    labelOptions.icon = icon();
    labelOptions.toolButtonStyle = Qt::ToolButtonTextBesideIcon;
    labelOptions.iconSize = iconSize();
    labelOptions.state.setFlag(QStyle::State_Sunken, isDown());
    painter.setOpacity(1);
    painter.drawControl(QStyle::CE_ToolButtonLabel, labelOptions);
}
