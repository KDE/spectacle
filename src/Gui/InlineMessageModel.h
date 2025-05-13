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
    static InlineMessageModel *instance();

    static InlineMessageModel *create(QQmlEngine *engine, QJSEngine *)
    {
        auto inst = instance();
        Q_ASSERT(inst);
        Q_ASSERT(inst->thread() == engine->thread());
        QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
        return inst;
    }

    enum CustomRoles {
        TypeRole = Qt::UserRole + 1,
        DataRole = Qt::UserRole + 2,
    };
    Q_ENUM(CustomRoles)

    enum InlineMessageType {
        // Warnings and errors don't replace others of the same enum value.
        Error,
        Warning,
        // Informational types replace others of the same enum value.
        InformationalType,
        Copied = InformationalType,
        Saved = InformationalType + 1,
        Shared = InformationalType + 2,
        Scanned = InformationalType + 3,
    };
    Q_ENUM(InlineMessageType)

    QHash<int, QByteArray> roleNames() const override;

    QVariant data(const QModelIndex &index, int role) const override;

    int rowCount(const QModelIndex &parent = {}) const override;

    Q_INVOKABLE void push(InlineMessageType type, const QString &text, const QVariant &data = {});
    Q_INVOKABLE void pop(int row = -1);
    Q_INVOKABLE void clear();

    Q_INVOKABLE void copyToClipboard(const QVariant &content);
    Q_INVOKABLE void openContainingFolder(const QUrl &url);

Q_SIGNALS:
    void countChanged();

private:
    InlineMessageModel(QObject *parent = nullptr);

    struct Item {
        InlineMessageType type;
        QString text;
        QVariant data;
    };

    QList<Item> m_data;
    QHash<int, QByteArray> m_roleNames;
};
