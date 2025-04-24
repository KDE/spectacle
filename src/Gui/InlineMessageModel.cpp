/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "InlineMessageModel.h"

using namespace Qt::StringLiterals;

static std::unique_ptr<InlineMessageModel> s_instance;

InlineMessageModel *InlineMessageModel::instance()
{
    if (!s_instance) {
        s_instance = std::make_unique<InlineMessageModel>();
    }
    return s_instance.get();
}

InlineMessageModel::InlineMessageModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_roleNames({{QmlFileRole, "qmlFile"_ba}, {PropertiesRole, "properties"_ba}})
{
}

QHash<int, QByteArray> InlineMessageModel::roleNames() const
{
    return m_roleNames;
}

QVariant InlineMessageModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    QVariant ret;
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return ret;
    }
    if (role == QmlFileRole) {
        ret = m_data.at(row).qmlFile;
    } else if (role == PropertiesRole) {
        ret = m_data.at(row).properties;
    }
    return ret;
}

bool InlineMessageModel::setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles)
{
    const int count = m_data.size();
    const int row = index.isValid() ? std::min(index.row(), count) : count;
    const auto keys = roles.keys();
    if (row == count) {
        if (keys != QList<int>{QmlFileRole, PropertiesRole}) {
            return false;
        }
        beginInsertRows(index.parent(), row, row);
        m_data.push_back({roles[QmlFileRole].toString(), roles[PropertiesRole].toMap()});
        endInsertRows();
        Q_EMIT dataChanged(index, index, keys);
        Q_EMIT countChanged();
        return true;
    }

    for (auto it = roles.constKeyValueBegin(); it != roles.constKeyValueEnd(); ++it) {
        if (it->first == QmlFileRole) {
            auto value = it->second.toString();
            if (m_data[row].qmlFile == value) {
                return false;
            }
            m_data[row].qmlFile = value;
        } else if (it->first == PropertiesRole) {
            auto value = it->second.toMap();
            if (m_data[row].properties == value) {
                return false;
            }
            m_data[row].properties = value;
        }
    }
    Q_EMIT dataChanged(index, index, keys);
    return true;
}

int InlineMessageModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_data.size();
}

bool InlineMessageModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent)
    if (row < 0 || row + count - 1 >= m_data.size()) {
        return false;
    }
    beginRemoveRows(parent, row, row + count - 1);
    m_data.remove(row, count);
    endRemoveRows();
    return true;
}

#include "moc_InlineMessageModel.cpp"
