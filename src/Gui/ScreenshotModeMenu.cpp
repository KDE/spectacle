/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ScreenshotModeMenu.h"
#include "CaptureModeModel.h"
#include "SpectacleCore.h"
#include "ShortcutActions.h"
#include <KGlobalAccel>

using namespace Qt::StringLiterals;

static QPointer<ScreenshotModeMenu> s_instance = nullptr;

ScreenshotModeMenu::ScreenshotModeMenu(QWidget *parent)
    : SpectacleMenu(i18nc("@title:menu", "Screenshot Modes"), parent)
{
    auto addModes = [this] {
        clear();
        auto model = CaptureModeModel::instance();
        for (auto idx = model->index(0); idx.isValid(); idx = idx.siblingAtRow(idx.row() + 1)) {
            const auto action = addAction(idx.data(Qt::DisplayRole).toString());
            const auto mode = idx.data(CaptureModeModel::CaptureModeRole).value<CaptureModeModel::CaptureMode>();
            QAction *globalAction = nullptr;
            auto globalShortcuts = [](QAction *globalAction) {
                if (!globalAction) {
                    return QList<QKeySequence>{};
                }
                auto component = ShortcutActions::self()->componentName();
                auto id = globalAction->objectName();
                return KGlobalAccel::self()->globalShortcut(component, id);
            };
            switch (mode) {
            case CaptureModeModel::RectangularRegion:
                globalAction = ShortcutActions::self()->regionAction();
                break;
            case CaptureModeModel::AllScreens:
                globalAction = ShortcutActions::self()->fullScreenAction();
                break;
            case CaptureModeModel::CurrentScreen:
                globalAction = ShortcutActions::self()->currentScreenAction();
                break;
            case CaptureModeModel::ActiveWindow:
                globalAction = ShortcutActions::self()->activeWindowAction();
                break;
            case CaptureModeModel::WindowUnderCursor:
                globalAction = ShortcutActions::self()->windowUnderCursorAction();
                break;
            case CaptureModeModel::FullScreen:
                globalAction = ShortcutActions::self()->fullScreenAction();
                break;
            default:
                break;
            }
            action->setShortcuts(globalShortcuts(globalAction));
            auto onTriggered = [mode] {
                SpectacleCore::instance()->takeNewScreenshot(mode);
            };
            connect(action, &QAction::triggered, action, onTriggered);
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
