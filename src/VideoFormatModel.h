/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "Platforms/VideoPlatform.h"

#include <QAbstractListModel>

/**
 * This is a model containing the current supported capture modes and their labels and shortcuts.
 */
class VideoFormatModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)
public:
    explicit VideoFormatModel(QObject *parent = nullptr);

    enum {
        FormatRole = Qt::UserRole + 1,
        ExtensionRole = Qt::UserRole + 2,
    };

    void setFormats(VideoPlatform::Formats formats);

    int indexOfFormat(VideoPlatform::Format format) const;

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

Q_SIGNALS:
    void countChanged();

private:
    struct Item {
        QString label;
        VideoPlatform::Format format = VideoPlatform::NoFormat;
        QString extension = {};
    };

    QList<Item> m_data;
    QHash<int, QByteArray> m_roleNames;
};
