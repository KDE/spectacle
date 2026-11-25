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
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

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

    enum Roles {
        DisplayRole = Qt::DisplayRole,
        DataRole = Qt::UserRole + 1,
        TypeRole,
        SubTypeRole,
        ControlsRole,
        BehaviorsRole,
    };
    Q_ENUM(Roles)

    // Kirigami InlineMessage types
    enum class Type {
        Information = 0,
        Positive,
        Error,
        Warning,
    };
    Q_ENUM(Type)

    enum class SubType {
        Generic = 0,
        Copy,
        Save,
        Share,
        Scan,
        OCR,
    };
    Q_ENUM(SubType)

    enum class Control {
        Close = 0, // always show close button
        OpenContainingFolder = 1,
        Copy = 1 << 1,
        Busy = 1 << 2,
        ControlMask = OpenContainingFolder | Copy | Busy,
    };
    Q_DECLARE_FLAGS(Controls, Control)
    Q_FLAG(Control)

    enum class Behavior {
        None = 0,
        Timeout = 1,
        Delay = 1 << 1,
        BehaviorMask = Timeout | Delay,
    };
    Q_DECLARE_FLAGS(Behaviors, Behavior)
    Q_FLAG(Behavior)

    struct Item {
        QString text;
        QVariant data{};
        Type type{Type::Information};
        SubType subtype{SubType::Generic};
        Controls controls{Control::Close};
        Behaviors behaviors{Behavior::None};
        bool operator==(const Item &other) const = default;
    };

    QHash<int, QByteArray> roleNames() const override;

    QVariant data(const QModelIndex &index, int role) const override;

    int rowCount(const QModelIndex &parent = {}) const override;

    Q_INVOKABLE void push(Item item);
    Q_INVOKABLE void pop(int row = -1);
    Q_INVOKABLE void clear();

    template<typename Predicate>
    QModelIndexList matchIf(Predicate pred) const;

    Q_INVOKABLE void copyToClipboard(const QVariant &content);
    Q_INVOKABLE void openContainingFolder(const QUrl &url);

Q_SIGNALS:
    void countChanged();

private:
    InlineMessageModel(QObject *parent = nullptr);

    QList<Item> m_data;
    QHash<int, QByteArray> m_roleNames;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(InlineMessageModel::Controls)
