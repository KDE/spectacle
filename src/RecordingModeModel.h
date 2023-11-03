/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "Platforms/VideoPlatform.h"

#include <QAbstractListModel>

class RecordingModeModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)
public:
    RecordingModeModel(VideoPlatform::RecordingModes modes, QObject *parent = nullptr);

    enum {
        RecordingModeRole = Qt::UserRole + 1,
    };

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    int indexOfRecordingMode(VideoPlatform::RecordingMode mode) const;

Q_SIGNALS:
    void countChanged();

private:
    struct Item {
        VideoPlatform::RecordingMode mode;
        QString label;
    };

    QList<Item> m_data;
    QHash<int, QByteArray> m_roleNames;
    const VideoPlatform::RecordingModes m_modes;
};
