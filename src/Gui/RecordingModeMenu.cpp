/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "RecordingModeMenu.h"
#include "RecordingModeModel.h"
#include "SpectacleCore.h"
#include "ShortcutActions.h"
#include <KGlobalAccel>

using namespace Qt::StringLiterals;

static QPointer<RecordingModeMenu> s_instance = nullptr;

RecordingModeMenu::RecordingModeMenu(QWidget *parent)
    : SpectacleMenu(i18nc("@title:menu", "Recording Modes"), parent)
{
    auto addModes = [this] {
        clear();
        auto model = RecordingModeModel::instance();
        for (auto idx = model->index(0); idx.isValid(); idx = idx.siblingAtRow(idx.row() + 1)) {
            const auto action = addAction(idx.data(Qt::DisplayRole).toString());
            const auto mode = idx.data(RecordingModeModel::RecordingModeRole).value<VideoPlatform::RecordingMode>();
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
            case VideoPlatform::Region:
                globalAction = ShortcutActions::self()->recordRegionAction();
                break;
            case VideoPlatform::Screen:
                globalAction = ShortcutActions::self()->recordScreenAction();
                break;
            case VideoPlatform::Window:
                globalAction = ShortcutActions::self()->recordWindowAction();
                break;
            default:
                break;
            }
            action->setShortcuts(globalShortcuts(globalAction));
            auto onTriggered = [mode] {
                SpectacleCore::instance()->startRecording(mode);
            };
            connect(action, &QAction::triggered, action, onTriggered);
        }
    };
    addModes();
    connect(RecordingModeModel::instance(), &RecordingModeModel::recordingModesChanged, this, addModes);
}

RecordingModeMenu *RecordingModeMenu::instance()
{
    if (!s_instance) {
        s_instance = new RecordingModeMenu;
    }
    return s_instance;
}

#include "moc_RecordingModeMenu.cpp"
