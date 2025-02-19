/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ScreenshotModeMenu.h"
#include "CaptureModeModel.h"
#include "SpectacleCore.h"

using namespace Qt::StringLiterals;

static QPointer<ScreenshotModeMenu> s_instance = nullptr;

ScreenshotModeMenu::ScreenshotModeMenu(QWidget *parent)
    : SpectacleMenu(i18nc("@title:menu", "Screenshot Modes"), parent)
{
    auto addModes = [this] {
        clear();
        auto model = CaptureModeModel::instance();
        for (auto idx = model->index(0); idx.isValid(); idx = idx.siblingAtRow(idx.row() + 1)) {
            const auto label = idx.data(Qt::DisplayRole).toString();
            const auto mode = idx.data(CaptureModeModel::CaptureModeRole).value<CaptureModeModel::CaptureMode>();
            addAction(label, [mode] {
                SpectacleCore::instance()->takeNewScreenshot(mode);
            });
        }
    };
    addModes();
    connect(CaptureModeModel::instance(), &CaptureModeModel::captureModesChanged, this, addModes);
}

ScreenshotModeMenu *ScreenshotModeMenu::instance()
{
    if (!s_instance) {
        s_instance = new ScreenshotModeMenu;
    }
    return s_instance;
}

#include "moc_ScreenshotModeMenu.cpp"
