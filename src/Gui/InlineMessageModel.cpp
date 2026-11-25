/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "InlineMessageModel.h"

#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenFileManagerWindowJob>
#include <KSystemClipboard>
#include <QTimer>
#include <QMimeData>

using namespace Qt::StringLiterals;

static std::unique_ptr<InlineMessageModel> s_instance;

InlineMessageModel *InlineMessageModel::instance()
{
    if (!s_instance) {
        s_instance = std::unique_ptr<InlineMessageModel>(new InlineMessageModel());
    }
    return s_instance.get();
}

InlineMessageModel::InlineMessageModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_roleNames({
          {DisplayRole, "text"_ba},
          {DataRole, "data"_ba},
          {TypeRole, "type"_ba},
          {SubTypeRole, "subtype"_ba},
          {ControlsRole, "controls"_ba},
          {BehaviorsRole, "behaviors"_ba},
      })
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
    if (role == Qt::DisplayRole) {
        ret = m_data.at(row).text;
    } else if (role == DataRole) {
        ret = m_data.at(row).data;
    } else if (role == TypeRole) {
        ret = QVariant::fromValue(m_data.at(row).type);
    } else if (role == SubTypeRole) {
        ret = QVariant::fromValue(m_data.at(row).subtype);
    } else if (role == ControlsRole) {
        ret = QVariant::fromValue(m_data.at(row).controls);
    } else if (role == BehaviorsRole) {
        ret = QVariant::fromValue(m_data.at(row).behaviors);
    }
    return ret;
}

int InlineMessageModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_data.size();
}

void InlineMessageModel::push(Item item)
{
    const bool isInformation = item.type == Type::Information;
    const bool isGeneric = item.subtype == SubType::Generic;
    const auto removed = matchIf([&](const Item &i) {
        const bool sameSubType = i.subtype == item.subtype;
        // Replace messages with the same non-generic sub-type.
        return (!isGeneric && sameSubType)
            // Replace generic info with other generic info
            || (isGeneric && isInformation && i.type == item.type && sameSubType)
            // Replace other messages that are exactly the same to prevent spam.
            || (item.subtype != SubType::Generic && i.subtype == item.subtype) || (i == item);
    });
    const int oldCount = m_data.size();
    // Remove in reverse to prevent invalid access
    for (auto it = removed.crbegin(); it != removed.crend(); ++it) {
        beginRemoveRows(it->parent(), it->row(), it->row());
        m_data.removeAt(it->row());
        endRemoveRows();
    }
    const int i = m_data.size();
    beginInsertRows({}, i, i);
    m_data.emplace_back(std::forward<Item>(item));
    endInsertRows();
    auto last = index(i);
    if (!removed.empty()) {
        Q_EMIT dataChanged(std::min(removed.front(), last), last, {DisplayRole, DataRole, TypeRole, SubTypeRole, ControlsRole, BehaviorsRole});
    }
    if (oldCount != m_data.size()) {
        Q_EMIT countChanged();
    }
}

void InlineMessageModel::pop(int row)
{
    if (row == -1) {
        row = m_data.size() - 1;
    }
    auto modelIndex = index(row);
    if (!modelIndex.isValid()) {
        return;
    }

    beginRemoveRows({}, row, row);
    m_data.removeAt(row);
    endRemoveRows();
    Q_EMIT countChanged();
}

void InlineMessageModel::clear()
{
    if (m_data.empty()) {
        return;
    }

    beginRemoveRows({}, 0, m_data.size() - 1);
    m_data = {};
    endRemoveRows();
    Q_EMIT countChanged();
}

template<typename Predicate>
QModelIndexList InlineMessageModel::matchIf(Predicate pred) const
{
    const auto oldCount = m_data.size();
    QModelIndexList indices;
    for (qsizetype i = 0; i < oldCount; ++i) {
        if (pred(m_data[i])) {
            indices.push_back(index(i));
        }
    }
    return indices;
}

void InlineMessageModel::copyToClipboard(const QVariant &content)
{
    auto data = new QMimeData();
    if (content.typeId() == QMetaType::QUrl) {
        data->setUrls({content.toUrl()});
    } else if (content.typeId() == QMetaType::QString) {
        data->setText(content.toString());
    } else if (content.typeId() == QMetaType::QByteArray) {
        data->setData(QStringLiteral("application/octet-stream"), content.toByteArray());
    } else {
        qWarning() << "copyToClipboard: Unhandled content type" << content.typeId() << content.typeName();
    }
    KSystemClipboard::instance()->setMimeData(data, QClipboard::Clipboard);
}

void InlineMessageModel::openContainingFolder(const QUrl &url)
{
    KIO::highlightInFileManager({url});
}

#include "moc_InlineMessageModel.cpp"
