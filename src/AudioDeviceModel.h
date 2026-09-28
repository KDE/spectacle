/* SPDX-FileCopyrightText: 2026 tomek7667 <git@cyber-man.pl>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "Platforms/AudioDeviceMonitor.h"

#include <QAbstractListModel>
#include <QQmlEngine>

/**
 * The audio devices that can be recorded along with a screen recording and
 * which of them are selected. Everything that is selected gets mixed into the
 * audio track of the recording, nothing selected means no audio.
 *
 * The first two rows follow the default output and input devices, followed by
 * the output devices and the input devices that are present. Selected devices
 * that are missing, e.g. an unplugged headset, are listed too so that they can
 * be deselected.
 */
class AudioDeviceModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString summary READ summary NOTIFY summaryChanged FINAL)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged FINAL)
    Q_PROPERTY(QString iconName READ iconName NOTIFY selectionChanged FINAL)

public:
    using DeviceType = AudioDeviceMonitor::DeviceType;

    enum Role {
        DeviceTypeRole = Qt::UserRole + 1,
        /// Empty for the rows following the default devices
        NodeNameRole,
        /// False for a selected device that is missing
        ConnectedRole,
    };

    struct Selection {
        bool defaultOutput = false;
        bool defaultInput = false;
        /// Node names of the Audio/Sink nodes
        QStringList outputs;
        /// Node names of the Audio/Source nodes
        QStringList inputs;

        bool isEmpty() const;
        /// The order in which the devices were selected doesn't matter
        bool operator==(const Selection &other) const;

        static Selection fromSettings();
        static Selection defaults();
        void saveToSettings() const;
    };

    explicit AudioDeviceModel(QObject *parent = nullptr);

    /**
     * The model shared by the recording options, whose selection is loaded
     * from and immediately written to the settings.
     */
    static AudioDeviceModel *instance();

    static AudioDeviceModel *create(QQmlEngine *engine, QJSEngine *)
    {
        auto inst = instance();
        Q_ASSERT(inst);
        Q_ASSERT(inst->thread() == engine->thread());
        QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
        return inst;
    }

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    Selection selection() const;
    void setSelection(const Selection &selection);

    bool hasSelection() const;
    /// A short description of what gets recorded, e.g. for a button opening the list
    QString summary() const;
    /// An icon showing whether anything gets recorded
    QString iconName() const;

Q_SIGNALS:
    void selectionChanged();
    void summaryChanged();

private:
    struct Item {
        DeviceType type;
        QString nodeName;
        QString label;
        bool connected;
    };

    void rebuild();
    bool isSelected(const Item &item) const;
    void setSelected(const Item &item, bool selected);

    QList<Item> m_items;
    Selection m_selection;
};
