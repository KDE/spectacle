/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "RecordingModeModel.h"
#include "ShortcutActions.h"

#include <KGlobalAccel>
#include <KLocalizedString>

#include "SpectacleCore.h"
#include <QApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <qnamespace.h>
#include <utility>

RecordingModeModel::RecordingModeModel(VideoPlatform::RecordingModes modes, QObject *parent)
    : QAbstractListModel(parent)
    , m_modes(modes)
{
    m_roleNames[RecordingModeRole] = QByteArrayLiteral("recordingMode");
    m_roleNames[Qt::DisplayRole] = QByteArrayLiteral("display");

    if (modes & VideoPlatform::Region) {
        m_data.append({VideoPlatform::Region, i18n("Workspace")}); // TODO: Rename to region when regions can be selected
    }

    if (modes & VideoPlatform::Region) {
        m_data.append({
            VideoPlatform::Screen,
            i18n("Selected Screen"),
        });
    }
    if (modes & VideoPlatform::Window) {
        m_data.append({
            VideoPlatform::Window,
            i18n("Selected Window"),
        });
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

void RecordingModeModel::startRecording(int row, bool withPointer)
{
    switch (m_data[row].mode) {
    case VideoPlatform::Screen: {
        // We should probably come up with a better way of choosing outputs. This should be okay for now. #FLW
        QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
                                                              QStringLiteral("/KWin"),
                                                              QStringLiteral("org.kde.KWin"),
                                                              QStringLiteral("queryWindowInfo"));

        QDBusPendingReply<QVariantMap> asyncReply = QDBusConnection::sessionBus().asyncCall(message);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(asyncReply, this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [withPointer, asyncReply](QDBusPendingCallWatcher *self) {
            self->deleteLater();
            if (!self->isValid()) {
                qWarning() << "error when querying window for output" << self->error();
                SpectacleCore::instance()->showErrorMessage(i18n("Failed to select output"));
                return;
            }

            const QVariantMap data = asyncReply.value();
            const QPoint top(data[QStringLiteral("x")].toDouble(), data[QStringLiteral("y")].toDouble());
            for (auto screen : qGuiApp->screens()) {
                if (screen->geometry().contains(top)) {
                    SpectacleCore::instance()->startRecordingScreen(screen, withPointer);
                    return;
                }
            }
        });
        break;
    }
    case VideoPlatform::Window: {
        QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
                                                              QStringLiteral("/KWin"),
                                                              QStringLiteral("org.kde.KWin"),
                                                              QStringLiteral("queryWindowInfo"));

        QDBusPendingReply<QVariantMap> asyncReply = QDBusConnection::sessionBus().asyncCall(message);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(asyncReply, this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [withPointer](QDBusPendingCallWatcher *self) {
            QDBusPendingReply<QVariantMap> reply = *self;
            self->deleteLater();
            if (!reply.isValid()) {
                qWarning() << "error when querying window" << self->error();
                SpectacleCore::instance()->showErrorMessage(i18n("Failed to select window"));
                return;
            }
            SpectacleCore::instance()->startRecordingWindow(reply.value().value(QStringLiteral("uuid")).toString(), withPointer);
        });
        break;
    }
    case VideoPlatform::Region: {
        // TODO: Ask user for the region
        QRect region;
        for (auto screen : qGuiApp->screens()) {
            region |= screen->geometry();
        }
        SpectacleCore::instance()->startRecordingRegion(region, withPointer);
        break;
    }
    }
}
