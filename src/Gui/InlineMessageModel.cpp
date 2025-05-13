/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "InlineMessageModel.h"

#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenFileManagerWindowJob>
#include <KSystemClipboard>
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
          {TypeRole, "type"_ba},
          {Qt::DisplayRole, "text"_ba},
          {DataRole, "data"_ba},
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
    if (role == TypeRole) {
        ret = m_data.at(row).type;
    } else if (role == Qt::DisplayRole) {
        ret = m_data.at(row).text;
    } else if (role == DataRole) {
        ret = m_data.at(row).data;
    }
    return ret;
}

int InlineMessageModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_data.size();
}

void InlineMessageModel::push(InlineMessageType type, const QString &text, const QVariant &data)
{
    const int oldCount = m_data.size();
    QModelIndex removed;
    if (type >= InformationalType) {
        for (int i = 0; i < oldCount; ++i) {
            if (m_data[i].type == type) {
                removed = index(i);
                beginRemoveRows({}, i, i);
                m_data.removeAt(i);
                endRemoveRows();
                break;
            }
        }
    }
    const int i = m_data.size();
    beginInsertRows({}, i, i);
    m_data.push_back({type, text, data});
    endInsertRows();
    auto last = index(i);
    if (removed.isValid()) {
        Q_EMIT dataChanged(removed, last, {TypeRole, Qt::DisplayRole, DataRole});
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

void InlineMessageModel::copyToClipboard(const QVariant &content)
{
    auto data = new QMimeData();
    if (content.typeId() == QMetaType::QString) {
        data->setText(content.toString());
    } else if (content.typeId() == QMetaType::QByteArray) {
        data->setData(QStringLiteral("application/octet-stream"), content.toByteArray());
    }
    KSystemClipboard::instance()->setMimeData(data, QClipboard::Clipboard);
}

void InlineMessageModel::openContainingFolder(const QUrl &url)
{
    KIO::highlightInFileManager({url});
}

#include "moc_InlineMessageModel.cpp"
