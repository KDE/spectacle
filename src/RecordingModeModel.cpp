/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "RecordingModeModel.h"
#include "ShortcutActions.h"
#include "SpectacleCore.h"
#include "Gui/SpectacleWindow.h"

#include <KGlobalAccel>
#include <KLocalizedString>

#include <QApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QScreen>
#include <qnamespace.h>
#include <utility>

using namespace Qt::StringLiterals;

RecordingModeModel::RecordingModeModel(VideoPlatform::RecordingModes modes, QObject *parent)
    : QAbstractListModel(parent)
    , m_modes(modes)
{
    m_roleNames[RecordingModeRole] = "recordingMode"_ba;
    m_roleNames[Qt::DisplayRole] = "display"_ba;

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

static void minimizeIfWindowsIntersect(const QRectF &rect) {
    const auto &windows = SpectacleWindow::instances();
    for (const auto window : windows) {
        if (rect.intersects(window->frameGeometry())) {
            SpectacleWindow::setVisibilityForAll(QWindow::Minimized);
            return;
        }
    }
}

void RecordingModeModel::startRecording(int row, bool withPointer)
{
    switch (m_data[row].mode) {
    case VideoPlatform::Screen: {
        // We should probably come up with a better way of choosing outputs. This should be okay for now. #FLW
        QDBusMessage message = QDBusMessage::createMethodCall(u"org.kde.KWin"_s,
                                                              u"/KWin"_s,
                                                              u"org.kde.KWin"_s,
                                                              u"queryWindowInfo"_s);

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
            const QPoint top(data[u"x"_s].toDouble(), data[u"y"_s].toDouble());
            const auto screens = qGuiApp->screens();
            for (auto screen : screens) {
                const auto &screenRect = screen->geometry();
                if (screenRect.contains(top)) {
                    minimizeIfWindowsIntersect(screenRect);
                    SpectacleCore::instance()->startRecordingScreen(screen, withPointer);
                    return;
                }
            }
        });
        break;
    }
    case VideoPlatform::Window: {
        QDBusMessage message = QDBusMessage::createMethodCall(u"org.kde.KWin"_s,
                                                              u"/KWin"_s,
                                                              u"org.kde.KWin"_s,
                                                              u"queryWindowInfo"_s);

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
            const auto &data = reply.value();
            // HACK: Window geometry from queryWindowInfo is from KWin's Window::frameGeometry(),
            // which may not be the same as QWindow::frameGeometry() on Wayland.
            // Hopefully this is good enough most of the time.
            const QRectF pickedWindowRect = {
                data[u"x"_s].toDouble(), data[u"y"_s].toDouble(),
                data[u"width"_s].toDouble(), data[u"height"_s].toDouble(),
            };
            // Don't minimize if we're recording Spectacle.
            if (data[u"desktopFile"_s].toString() != qGuiApp->desktopFileName()) {
                minimizeIfWindowsIntersect(pickedWindowRect);
            }
            SpectacleCore::instance()->startRecordingWindow(data.value(u"uuid"_s).toString(), withPointer);
        });
        break;
    }
    case VideoPlatform::Region: {
        // TODO: Ask user for the region
        QRect region;
        const auto screens = qGuiApp->screens();
        for (auto screen : screens) {
            region |= screen->geometry();
        }
        minimizeIfWindowsIntersect(region);
        SpectacleCore::instance()->startRecordingRegion(region, withPointer);
        break;
    }
    }
}

#include "moc_RecordingModeModel.cpp"
