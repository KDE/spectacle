/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "ExportManager.h"

#include <KLocalizedString>

#include <QGuiApplication>
#include <QLabel>

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
