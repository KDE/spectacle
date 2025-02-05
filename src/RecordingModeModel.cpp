/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "RecordingModeModel.h"
#include "SpectacleCore.h"

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

static std::unique_ptr<RecordingModeModel> s_instance;

RecordingModeModel *RecordingModeModel::instance()
{
    if (!s_instance) {
        s_instance = std::make_unique<RecordingModeModel>();
    }
    return s_instance.get();
}

RecordingModeModel::RecordingModeModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_roleNames[RecordingModeRole] = "recordingMode"_ba;
    m_roleNames[Qt::DisplayRole] = "display"_ba;

    auto platform = SpectacleCore::instance()->videoPlatform();
    connect(platform, &VideoPlatform::supportedRecordingModesChanged, this, [this, platform]() {
        setRecordingModes(platform->supportedRecordingModes());
    });
    setRecordingModes(platform->supportedRecordingModes());
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

void RecordingModeModel::setRecordingModes(VideoPlatform::RecordingModes modes)
{
    auto count = m_data.size();
    m_data.clear();
    if (modes & VideoPlatform::Region) {
        m_data.append({VideoPlatform::Region, recordingModeLabel(VideoPlatform::Region)});
    }
    if (modes & VideoPlatform::Screen) {
        m_data.append({VideoPlatform::Screen, recordingModeLabel(VideoPlatform::Screen)});
    }
    if (modes & VideoPlatform::Window) {
        m_data.append({VideoPlatform::Window, recordingModeLabel(VideoPlatform::Window)});
    }
    Q_EMIT recordingModesChanged();
    if (count != m_data.size()) {
        Q_EMIT countChanged();
    }
}

QString RecordingModeModel::recordingModeLabel(VideoPlatform::RecordingMode mode)
{
    switch (mode) {
    case VideoPlatform::RecordingMode::Region:
        return i18nc("@item recording mode", "Rectangular Region");
    case VideoPlatform::RecordingMode::Window:
        return i18nc("@item recording mode", "Window");
    case VideoPlatform::RecordingMode::Screen:
        return i18nc("@item recording mode", "Full Screen");
    case VideoPlatform::RecordingMode::NoRecordingModes:
        break;
    }
    return QString{};
}

#include "moc_RecordingModeModel.cpp"
