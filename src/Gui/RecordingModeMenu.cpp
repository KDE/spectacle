/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "RecordingModeMenu.h"
#include "RecordingModeModel.h"
#include "SpectacleCore.h"

using namespace Qt::StringLiterals;

static QPointer<RecordingModeMenu> s_instance = nullptr;

RecordingModeMenu::RecordingModeMenu(QWidget *parent)
    : SpectacleMenu(i18nc("@title:menu", "Recording Modes"), parent)
{
    auto addModes = [this] {
        clear();
        auto model = RecordingModeModel::instance();
        for (auto idx = model->index(0); idx.isValid(); idx = idx.siblingAtRow(idx.row() + 1)) {
            const auto label = idx.data(Qt::DisplayRole).toString();
            const auto mode = idx.data(RecordingModeModel::RecordingModeRole).value<VideoPlatform::RecordingMode>();
            addAction(label, [mode] {
                SpectacleCore::instance()->startRecording(mode);
            });
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
