/* SPDX-FileCopyrightText: 2026 tomek7667 <git@cyber-man.pl>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "AudioDeviceMenu.h"

#include "AudioDeviceModel.h"
#include "SpectacleCore.h"

#include <KLocalizedString>

#include <QApplication>
#include <QPersistentModelIndex>

#include <optional>

using namespace Qt::StringLiterals;

static AudioDeviceMenu *s_instance = nullptr;

AudioDeviceMenu *AudioDeviceMenu::instance()
{
    if (!s_instance && SpectacleCore::instance()) {
        s_instance = new AudioDeviceMenu(AudioDeviceModel::instance());
        // Destroyed after SpectacleCore like the other menus, see HelpMenu::instance()
        connect(SpectacleCore::instance(), &QObject::destroyed, qApp, [] {
            delete s_instance;
        });
        connect(s_instance, &QObject::destroyed, qApp, [] {
            s_instance = nullptr;
        });
    }
    return s_instance;
}

AudioDeviceMenu::AudioDeviceMenu(AudioDeviceModel *model, QWidget *parent)
    : SpectacleMenu(i18nc("@title:menu", "Record Audio"), parent)
    , m_model(model)
{
    setToolTipsVisible(true);
    setKeepOpenOnCheckableActions(true);

    const auto updateIcon = [this] {
        setIcon(QIcon::fromTheme(m_model->iconName()));
    };
    updateIcon();
    connect(m_model, &AudioDeviceModel::selectionChanged, this, updateIcon);

    connect(m_model, &QAbstractItemModel::modelReset, this, &AudioDeviceMenu::rebuild);
    connect(m_model, &QAbstractItemModel::dataChanged, this, &AudioDeviceMenu::updateCheckedStates);
    rebuild();
}

void AudioDeviceMenu::rebuild()
{
    clear();
    m_deviceActions.clear();

    // Section headers for the output and the input devices, the rows following
    // the default devices come first and don't need one.
    std::optional<AudioDeviceModel::DeviceType> section;
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const QPersistentModelIndex index = m_model->index(row);
        const auto type = static_cast<AudioDeviceModel::DeviceType>(index.data(AudioDeviceModel::DeviceTypeRole).toInt());
        if (!index.data(AudioDeviceModel::NodeNameRole).toString().isEmpty() && section != type) {
            section = type;
            addSection(type == AudioDeviceMonitor::Output ? i18nc("@title:menu", "Output Devices") : i18nc("@title:menu", "Input Devices"));
        }

        // Device names like "B&O" would otherwise get a mnemonic
        auto action = addAction(index.data(Qt::DisplayRole).toString().replace('&'_L1, "&&"_L1));
        action->setToolTip(index.data(Qt::ToolTipRole).toString());
        action->setCheckable(true);
        action->setChecked(index.data(Qt::CheckStateRole).toInt() == Qt::Checked);
        connect(action, &QAction::toggled, this, [this, index](bool checked) {
            if (index.isValid()) {
                m_model->setData(index, checked ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
            }
        });
        m_deviceActions.append(action);
    }
}

void AudioDeviceMenu::updateCheckedStates()
{
    for (int row = 0; row < m_deviceActions.size() && row < m_model->rowCount(); ++row) {
        const auto checked = m_model->index(row).data(Qt::CheckStateRole).toInt() == Qt::Checked;
        m_deviceActions.at(row)->setChecked(checked);
    }
}

#include "moc_AudioDeviceMenu.cpp"
