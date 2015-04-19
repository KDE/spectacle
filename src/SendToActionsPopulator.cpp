/*
 *  Copyright (C) 2015 Boudhayan Gupta <me@BaloneyGeek.com>
 *
 *  Contains code from ksnapshotsendtoactions.cpp, part of KSnapshot.
 *  Copyright notices reproduced below:
 *
 *  Copyright (C) 2014 Gregor Mi <codestruct@posteo.org>
 *  moved from ksnapshot.h, see authors there
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "SendToActionsPopulator.h"

SendToActionsPopulator::SendToActionsPopulator(QObject *parent) : QObject(parent)
{

}

SendToActionsPopulator::~SendToActionsPopulator()
{

}

void SendToActionsPopulator::process()
{
    sendHardcodedSendToActions();
    emit haveSeperator();
    sendKServiceSendToActions();
#ifdef KIPI_FOUND
    emit haveSeperator();
    sendKipiSendToActions();
#endif
    emit allDone();
}

void SendToActionsPopulator::sendHardcodedSendToActions()
{
    const QVariant data_clip = QVariant::fromValue(ActionData(HardcodedAction, "clipboard"));
    const QVariant data_app = QVariant::fromValue(ActionData(HardcodedAction, "application"));

    emit haveAction(QIcon::fromTheme("edit-copy"), i18n("Copy To Clipboard"), data_clip);
    emit haveAction(QIcon(), i18n("Other Application"), data_app);
}

void SendToActionsPopulator::sendKServiceSendToActions()
{
    const KService::List services = KMimeTypeTrader::self()->query("image/png");

    for (auto service: services) {
        QString name = service->name().replace('&', "&&");
        const QVariant data = QVariant::fromValue(ActionData(KServiceAction, service->menuId()));

        emit haveAction(QIcon::fromTheme(service->icon()), name, data);
    }
}

#ifdef KIPI_FOUND
void SendToActionsPopulator::sendKipiSendToActions()
{
    KIPI::PluginLoader *loader = new KIPI::PluginLoader;
    KIPI::PluginLoader::PluginList pluginList = loader->pluginList();

    for (auto pluginInfo: pluginList) {
        qDebug() << "Here";
        if (!(pluginInfo->shouldLoad())) {
            continue;
        }

        KIPI::Plugin *plugin = pluginInfo->plugin();
        if (!(plugin)) {
            qWarning() << i18n("KIPI plugin from library %1 failed to load", pluginInfo->library());
            continue;
        }

        plugin->setup(0);

        QList<QAction *> actions = plugin->actions();
        QSet<QAction *> exportActions;

        for (auto action: actions) {
            KIPI::Category category = plugin->category(action);
            if (category == KIPI::ExportPlugin) {
                exportActions += action;
            } else if (category == KIPI::ImagesPlugin) {
                // Horrible hack. Why are the print images and the e-mail images plugins in the same category as rotate and edit metadata!?
                // 2014-10-30: please file kipi bug and reference it here
                if (pluginInfo->library().contains("kipiplugin_printimages") || pluginInfo->library().contains("kipiplugin_sendimages")) {
                    exportActions += action;
                }
            }
        }

        for (QAction *action: exportActions) {
            emit haveAction(action->icon(), action->text(), QVariant::fromValue(ActionData(KipiAction, "some")));
        }

    }
}
#endif
