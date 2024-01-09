/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "ExportManager.h"

#include <KLocalizedString>

#include <QApplication>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QLabel>
#include <QStyle>

using namespace Qt::StringLiterals;

/**
 * A small collection of functions to help prevent duplicating the implementations of the
 * image and video save options pages.
 */

inline void updateFilenamePreview(QLabel *label, const QString &templateFilename)
{
    auto exportManager = ExportManager::instance();
    // If there is no window title, we need to change it to have a placeholder.
    const bool usePlaceholder = exportManager->windowTitle().isEmpty();
    if (usePlaceholder) {
        exportManager->setWindowTitle(QGuiApplication::applicationDisplayName());
    }
    const auto filename = exportManager->formattedFilename(templateFilename);
    label->setText(xi18nc("@info", "<filename>%1</filename>", filename));
    if (usePlaceholder) {
        exportManager->setWindowTitle({});
    }
}

namespace CaptureInstructionHelpers
{
inline QString tableRow(const QString &href, const QString &label, const QString &description = {})
{
    QString cell1 = u"<a href='%1'><code>%2</code></a>"_s.arg(href, label);
    // clang-format off
    return uR"(
            <tr><td>%1</td>
                <td>%2</td></tr>)"_s.arg(cell1, description);
    // clang-format on
}
inline QString buttonRow(const QString &href, const QString &label)
{
    QString cell2 = u"<a href='%1'>%2</a>"_s.arg(href, label);
    // clang-format off
    return uR"(
            <tr><td>&nbsp;</td>
                <td>%1</td></tr>)"_s.arg(cell2);
    // clang-format on
}
}

inline QString captureInstructions(bool showExtras)
{
    using namespace CaptureInstructionHelpers;
    QString intro = i18n("You can use the following placeholders in the filename, which will be replaced with actual text when the file is saved:");

    QString tableBody;
    bool hasAnyExtras = false;
    for (auto it = ExportManager::filenamePlaceholders.cbegin(); it != ExportManager::filenamePlaceholders.cend(); ++it) {
        using Flag = ExportManager::Placeholder::Flag;
        if (it->flags.testFlag(Flag::Hidden)) {
            continue;
        }
        const bool isExtra = it->flags.testFlag(Flag::Extra);
        hasAnyExtras |= isExtra;
        if (showExtras || !isExtra) {
            tableBody += tableRow(it->plainKey, it->htmlKey, it->description.toString());
        }
    }
    tableBody += tableRow(u"/"_s, u"/"_s, i18n("To save to a sub-folder"));
    if (hasAnyExtras) {
        if (showExtras) {
            tableBody += buttonRow(u"showfewer"_s, i18nc("show fewer filename placeholder templates", "Show Fewer"));
        } else {
            tableBody += buttonRow(u"showmore"_s, i18nc("show more filename placeholder templates", "Show More"));
        }
    }

    auto hspacing = qApp->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing);
    // Make the distance from the bottom of a typical capital letter to the top of another below it
    // equal to QLayout vertical spacing, unless it is less than the largest descent.
    qreal vspacing = qApp->style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing);
    QFontMetricsF generalMetrics(QFontDatabase::systemFont(QFontDatabase::GeneralFont));
    QFontMetricsF fixedMetrics(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    vspacing = std::max(0.0, vspacing - std::max(generalMetrics.descent(), fixedMetrics.descent()));

    static const QString html = // clang-format off
    // We simulate independent vertical and horizontal cell spacing by using padding for cells
    // and negative margins for the table.
uR"(<html>
    <head>
        <style>
            table {
                border: 0px none transparent;
                margin-left: -%1px;
                margin-right: -%1px;
                margin-top: -%2px;
                margin-bottom: -%2px;
            }
            td {
                border: 0px none transparent;
                padding-left: %1px;
                padding-right: %1px;
                padding-top: %2px;
                padding-bottom: %2px;
            }
        </style>
    </head>
    <body>
        <p>%3</p>
        <p><table cellspacing='0' cellpadding='0'>%4
        </table></p>
    </body>
</html>)"_s; // clang-format on
    return html.arg(QString::number(hspacing), QString::number(vspacing / 2.0), intro, tableBody);
}
