/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "Platforms/VideoPlatform.h"

#include <QAbstractListModel>
#include <QQmlEngine>

class RecordingModeModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)
public:
    explicit RecordingModeModel(QObject *parent = nullptr);

    static RecordingModeModel *instance();

    static RecordingModeModel *create(QQmlEngine *engine, QJSEngine *)
    {
        auto inst = instance();
        Q_ASSERT(inst);
        Q_ASSERT(inst->thread() == engine->thread());
        QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
        return inst;
    }

    enum {
        RecordingModeRole = Qt::UserRole + 1,
    };

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    int indexOfRecordingMode(VideoPlatform::RecordingMode mode) const;

    void setRecordingModes(VideoPlatform::RecordingModes modes);

    static QString recordingModeLabel(VideoPlatform::RecordingMode mode);

Q_SIGNALS:
    void countChanged();
    void recordingModesChanged();

private:
    struct Item {
        VideoPlatform::RecordingMode mode;
        QString label;
    };

    QList<Item> m_data;
    QHash<int, QByteArray> m_roleNames;
    const VideoPlatform::RecordingModes m_modes;
};
