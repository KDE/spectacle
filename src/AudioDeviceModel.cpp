/* SPDX-FileCopyrightText: 2026 tomek7667 <git@cyber-man.pl>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "AudioDeviceModel.h"
#include "settings.h"

#include <KLocalizedString>

#include <QCollator>
#include <QCoreApplication>

#include <algorithm>

using namespace Qt::StringLiterals;

bool AudioDeviceModel::Selection::isEmpty() const
{
    return !defaultOutput && !defaultInput && outputs.isEmpty() && inputs.isEmpty();
}

bool AudioDeviceModel::Selection::operator==(const Selection &other) const
{
    const auto sameDevices = [](QStringList a, QStringList b) {
        a.sort();
        b.sort();
        return a == b;
    };
    return defaultOutput == other.defaultOutput && defaultInput == other.defaultInput && sameDevices(outputs, other.outputs)
        && sameDevices(inputs, other.inputs);
}

AudioDeviceModel::Selection AudioDeviceModel::Selection::fromSettings()
{
    return {
        .defaultOutput = Settings::videoRecordSystemAudio(),
        .defaultInput = Settings::videoRecordMicrophone(),
        .outputs = Settings::videoRecordedAudioSinks(),
        .inputs = Settings::videoRecordedAudioSources(),
    };
}

AudioDeviceModel::Selection AudioDeviceModel::Selection::defaults()
{
    return {
        .defaultOutput = Settings::defaultVideoRecordSystemAudioValue(),
        .defaultInput = Settings::defaultVideoRecordMicrophoneValue(),
        // The device lists are empty by default
        .outputs = {},
        .inputs = {},
    };
}

void AudioDeviceModel::Selection::saveToSettings() const
{
    Settings::setVideoRecordSystemAudio(defaultOutput);
    Settings::setVideoRecordMicrophone(defaultInput);
    Settings::setVideoRecordedAudioSinks(outputs);
    Settings::setVideoRecordedAudioSources(inputs);
}

static AudioDeviceModel *s_instance = nullptr;

AudioDeviceModel *AudioDeviceModel::instance()
{
    if (!s_instance) {
        s_instance = new AudioDeviceModel(QCoreApplication::instance());
        s_instance->setSelection(Selection::fromSettings());

        // Saving writes the settings one by one. Each write reloads the
        // selection, which is a no-op once the settings caught up with it.
        const auto load = [] {
            s_instance->setSelection(Selection::fromSettings());
        };
        auto settings = Settings::self();
        connect(settings, &Settings::videoRecordSystemAudioChanged, s_instance, load);
        connect(settings, &Settings::videoRecordMicrophoneChanged, s_instance, load);
        connect(settings, &Settings::videoRecordedAudioSinksChanged, s_instance, load);
        connect(settings, &Settings::videoRecordedAudioSourcesChanged, s_instance, load);
        connect(s_instance, &AudioDeviceModel::selectionChanged, s_instance, [] {
            s_instance->selection().saveToSettings();
        });
        connect(s_instance, &QObject::destroyed, [] {
            s_instance = nullptr;
        });
    }
    return s_instance;
}

AudioDeviceModel::AudioDeviceModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(AudioDeviceMonitor::instance(), &AudioDeviceMonitor::devicesChanged, this, &AudioDeviceModel::rebuild);
    rebuild();
}

QHash<int, QByteArray> AudioDeviceModel::roleNames() const
{
    return {
        {Qt::DisplayRole, "display"_ba},
        {Qt::CheckStateRole, "checkState"_ba},
        {DeviceTypeRole, "deviceType"_ba},
        {NodeNameRole, "nodeName"_ba},
        {ConnectedRole, "connected"_ba},
    };
}

int AudioDeviceModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant AudioDeviceModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }
    const auto &item = m_items.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return item.label;
    case Qt::ToolTipRole:
        if (item.nodeName.isEmpty()) {
            return item.type == AudioDeviceMonitor::Output
                ? i18nc("@info:tooltip", "Record what is playing on the current default output device, even when the default changes.")
                : i18nc("@info:tooltip", "Record the current default input device, such as a microphone, even when the default changes.");
        }
        return item.nodeName;
    case Qt::CheckStateRole:
        return isSelected(item) ? Qt::Checked : Qt::Unchecked;
    case DeviceTypeRole:
        return item.type;
    case NodeNameRole:
        return item.nodeName;
    case ConnectedRole:
        return item.connected;
    }
    return {};
}

bool AudioDeviceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::CheckStateRole || !checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return false;
    }
    const auto &item = m_items.at(index.row());
    const bool selected = value.toInt() == Qt::Checked;
    if (isSelected(item) == selected) {
        return false;
    }
    // A missing device keeps its row after being deselected, it goes away on
    // the next rebuild. Removing it right away would move the row out from
    // under the pointer while the list is open.
    setSelected(item, selected);
    Q_EMIT dataChanged(index, index, {Qt::CheckStateRole});
    Q_EMIT selectionChanged();
    Q_EMIT summaryChanged();
    return true;
}

Qt::ItemFlags AudioDeviceModel::flags(const QModelIndex &index) const
{
    return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable;
}

AudioDeviceModel::Selection AudioDeviceModel::selection() const
{
    return m_selection;
}

void AudioDeviceModel::setSelection(const Selection &selection)
{
    if (m_selection == selection) {
        return;
    }
    m_selection = selection;
    // Selected devices that are missing are listed, so the rows can change
    rebuild();
    Q_EMIT selectionChanged();
}

bool AudioDeviceModel::hasSelection() const
{
    return !m_selection.isEmpty();
}

QString AudioDeviceModel::summary() const
{
    QStringList labels;
    for (const auto &item : m_items) {
        if (isSelected(item)) {
            labels.append(item.label);
        }
    }
    if (labels.isEmpty()) {
        return i18nc("@item no audio is recorded", "Muted");
    }
    if (labels.size() == 1) {
        return labels.constFirst();
    }
    return i18ncp("@item number of audio devices that are recorded", "%1 device", "%1 devices", labels.size());
}

QString AudioDeviceModel::iconName() const
{
    return hasSelection() ? u"audio-volume-high"_s : u"audio-volume-muted"_s;
}

void AudioDeviceModel::rebuild()
{
    const auto monitor = AudioDeviceMonitor::instance();
    const auto devices = monitor->devices();
    QList<Item> outputs;
    QList<Item> inputs;
    for (const auto &device : devices) {
        auto &items = device.type == AudioDeviceMonitor::Output ? outputs : inputs;
        // Different nodes can share a name, the recorder can't tell them apart either
        const bool duplicate = std::any_of(items.cbegin(), items.cend(), [&device](const Item &item) {
            return item.nodeName == device.nodeName;
        });
        if (!duplicate) {
            items.append({device.type, device.nodeName, device.description, true});
        }
    }

    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    const auto byLabel = [&collator](const Item &a, const Item &b) {
        return collator.compare(a.label, b.label) < 0;
    };
    std::sort(outputs.begin(), outputs.end(), byLabel);
    std::sort(inputs.begin(), inputs.end(), byLabel);

    const auto appendMissing = [monitor](QList<Item> &items, const QStringList &selected, DeviceType type) {
        for (const auto &nodeName : selected) {
            const bool present = std::any_of(items.cbegin(), items.cend(), [&nodeName](const Item &item) {
                return item.nodeName == nodeName;
            });
            if (!present && !nodeName.isEmpty()) {
                const auto label = i18nc("@item audio device that is not plugged in, %1 is its name", "%1 (disconnected)", monitor->description(type, nodeName));
                items.append({type, nodeName, label, false});
            }
        }
    };
    appendMissing(outputs, m_selection.outputs, AudioDeviceMonitor::Output);
    appendMissing(inputs, m_selection.inputs, AudioDeviceMonitor::Input);

    beginResetModel();
    m_items.clear();
    m_items.append({AudioDeviceMonitor::Output, {}, i18nc("@item audio device", "Default output"), true});
    m_items.append({AudioDeviceMonitor::Input, {}, i18nc("@item audio device", "Default input"), true});
    m_items.append(outputs);
    m_items.append(inputs);
    endResetModel();
    // The summary shows the labels, which can change with the devices
    Q_EMIT summaryChanged();
}

bool AudioDeviceModel::isSelected(const Item &item) const
{
    if (item.type == AudioDeviceMonitor::Output) {
        return item.nodeName.isEmpty() ? m_selection.defaultOutput : m_selection.outputs.contains(item.nodeName);
    }
    return item.nodeName.isEmpty() ? m_selection.defaultInput : m_selection.inputs.contains(item.nodeName);
}

void AudioDeviceModel::setSelected(const Item &item, bool selected)
{
    const bool isOutput = item.type == AudioDeviceMonitor::Output;
    if (item.nodeName.isEmpty()) {
        (isOutput ? m_selection.defaultOutput : m_selection.defaultInput) = selected;
        return;
    }
    auto &nodeNames = isOutput ? m_selection.outputs : m_selection.inputs;
    if (selected) {
        nodeNames.append(item.nodeName);
    } else {
        nodeNames.removeAll(item.nodeName);
    }
}

#include "moc_AudioDeviceModel.cpp"
