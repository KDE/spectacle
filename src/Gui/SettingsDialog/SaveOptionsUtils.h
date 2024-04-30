/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "ExportManager.h"

#include <KLocalizedString>

#include <QApplication>
#include <QFontDatabase>
#include <QLabel>
#include <QRegularExpression>
#include <QStyle>
#include <QToolTip>

using namespace Qt::StringLiterals;

/**
 * A small collection of functions to help prevent duplicating the implementations of the
 * image and video save options pages.
 */

namespace Filename
{
using QRE = QRegularExpression;

inline QString operator""_esc(const char16_t *str, size_t size) noexcept
{
    return QRE::escape(operator""_s(str, size).toHtmlEscaped());
}

// forward slash (/) is also incompatible, but that is interpreted as a dir separator.
// more than one forward slash should be considered incompatible.
static const auto FAT32LFN_exFAT_NTFS_Full_Str = u"/\\:*\"?<>|"_s;
static const QRegularExpression FAT32LFN_exFAT_NTFS_RegEx{u"((?>%1|[%2])+)"_s.arg(QRE::escape(u"//"_s), QRE::escape(u"\\:*\"?<>|"_s))};
static const QRegularExpression FAT32LFN_exFAT_NTFS_RegEx_HTML{
    u"((?>%1|%2|%3|%4|%5|%6|%7|%8|%9)+)"_s
        .arg(u"//"_esc, u"\\"_esc, u":"_esc, u"*"_esc, u"\""_esc, u"?"_esc, u"<"_esc, u">"_esc, u"|"_esc)};

inline QString previewText(const QString &templateFilename)
{
    auto exportManager = ExportManager::instance();
    // If there is no window title, we need to change it to have a placeholder.
    const bool usePlaceholder = exportManager->windowTitle().isEmpty();
    if (usePlaceholder) {
        exportManager->setWindowTitle(QGuiApplication::applicationDisplayName());
    }
    // Likewise, if no timestamp was set yet, we'll produce a new one as placeholder.
    const QDateTime timestamp = exportManager->timestamp();
    if (!timestamp.isValid()) {
        exportManager->updateTimestamp();
    }
    const auto filename = exportManager->formattedFilename(templateFilename);

    // Reset any previously empty values that we had temporarily set.
    if (!timestamp.isValid()) {
        exportManager->setTimestamp(timestamp);
    }
    if (usePlaceholder) {
        exportManager->setWindowTitle({});
    }

    return filename;
}

inline bool showWarning(const QString &text)
{
    return text.contains(FAT32LFN_exFAT_NTFS_RegEx);
}

inline QString highlightedPreviewText(QString text)
{
    return text.toHtmlEscaped().replace(FAT32LFN_exFAT_NTFS_RegEx_HTML, u"<u>\\1</u>"_s);
}

inline void warningToolTip(QWidget *target, bool show)
{
    Q_ASSERT(target != nullptr);
    // Depending on a bit of privately implemented behavior, but it probably won't ever change.
    QLabel *tooltipLabel = target->findChild<QLabel *>("qtooltip_label"_L1);
    // If the current tooltip is for the target and there is no warning, hide it immediately.
    if (!show && tooltipLabel) {
        tooltipLabel->close();
        return;
    }
    // If the current tooltip is not for the target, hide it immediately.
    // If we don't do this, the tooltip may not actually move back to the target position.
    if (!tooltipLabel) {
        const auto &widgets = qApp->topLevelWidgets();
        for (auto widget : widgets) {
            if ((widget->windowFlags() & Qt::WindowType_Mask) == Qt::ToolTip) {
                widget->close();
                break;
            }
        }
    }
    auto pos = target->mapToGlobal(QPoint{0, 0});
    auto warningText =
        i18nc("@info:tooltip",
              "The following characters are incompatible with Windows and removable storage devices file systems (NTFS, FAT32, exFAT):<br><b>%1</b>",
              FAT32LFN_exFAT_NTFS_Full_Str.toHtmlEscaped());
    QToolTip::showText(pos, warningText, target, {});
}
}

namespace CaptureInstructions
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

inline QString text(bool showExtras)
{
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
}
