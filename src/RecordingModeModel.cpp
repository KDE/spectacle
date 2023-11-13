/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "RecordingModeModel.h"

#include <KGlobalAccel>
#include <KLocalizedString>

#include <QApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QScreen>
#include <qnamespace.h>

using namespace Qt::StringLiterals;

RecordingModeModel::RecordingModeModel(VideoPlatform::RecordingModes modes, QObject *parent)
    : QAbstractListModel(parent)
    , m_modes(modes)
{
    m_roleNames[RecordingModeRole] = "recordingMode"_ba;
    m_roleNames[Qt::DisplayRole] = "display"_ba;

    if (modes & VideoPlatform::Region) {
        m_data.append({VideoPlatform::Region, i18nc("@item recording mode", "Rectangular Region")});
    }
    if (modes & VideoPlatform::Region) {
        m_data.append({VideoPlatform::Screen, i18nc("@item recording mode", "Full Screen")});
    }
    if (modes & VideoPlatform::Window) {
        m_data.append({VideoPlatform::Window, i18nc("@item recording mode", "Window")});
    }
}

QHash<int, QByteArray> RecordingModeModel::roleNames() const
{
    return m_roleNames;
}

QVariant RecordingModeModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    QVariant ret;
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return ret;
    }
    if (role == RecordingModeRole) {
        ret = m_data.at(row).mode;
    } else if (role == Qt::DisplayRole) {
        ret = m_data.at(row).label;
    }
    return ret;
}

int RecordingModeModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_data.size();
}

int RecordingModeModel::indexOfRecordingMode(VideoPlatform::RecordingMode mode) const
{
    int finalIndex = -1;
    for (int i = 0; i < m_data.length(); ++i) {
        if (m_data[i].mode == mode) {
            finalIndex = i;
            break;
        }
    }
    return finalIndex;
}

#include "moc_RecordingModeModel.cpp"
