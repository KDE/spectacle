/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>

/**
 * This is a model containing the current supported capture modes and their labels and shortcuts.
 */
class InlineMessageModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

public:
    InlineMessageModel(QObject *parent = nullptr);

    static InlineMessageModel *instance();

    static InlineMessageModel *create(QQmlEngine *engine, QJSEngine *)
    {
        auto inst = instance();
        Q_ASSERT(inst);
        Q_ASSERT(inst->thread() == engine->thread());
        QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
        return inst;
    }

    enum {
        QmlFileRole = Qt::UserRole + 1,
        PropertiesRole = Qt::UserRole + 2,
    };

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles) override;
    int rowCount(const QModelIndex &parent = {}) const override;
    bool removeRows(int row, int count, const QModelIndex &parent = {}) override;

Q_SIGNALS:
    void countChanged();

private:
    struct Item {
        QString qmlFile;
        QVariantMap properties;
    };

    QList<Item> m_data;
    QHash<int, QByteArray> m_roleNames;
};
