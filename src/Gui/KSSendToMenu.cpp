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

#include "KSSendToMenu.h"

KSSendToMenu::KSSendToMenu(QObject *parent) :
    QObject(parent),
    mMenu(new QMenu)
{}

KSSendToMenu::~KSSendToMenu()
{}

void KSSendToMenu::populateMenu()
{
    populateHardcodedSendToActions();
    mMenu->addSeparator();
    populateKServiceSendToActions();
#ifdef KIPI_FOUND
    mMenu->addSeparator();
    populateKipiSendToActions();
#endif
}

// return menu

QMenu *KSSendToMenu::menu()
{
    return mMenu;
}

// send-to handlers

void KSSendToMenu::handleSendToKService()
{
    QAction *action = qobject_cast<QAction *>(QObject::sender());
    if (!(action)) {
        qWarning() << "Internal qobject_cast error. This is a bug.";
        return;
    }

    auto data = action->data().value<KService::Ptr>();
    emit sendToServiceRequest(data);
}

// populators

void KSSendToMenu::populateHardcodedSendToActions()
{
    mMenu->addAction(QIcon::fromTheme("edit-copy"), i18n("Copy To Clipboard"), this, SIGNAL(sendToClipboardRequest()));
    mMenu->addAction(i18n("Other Application"), this, SIGNAL(sendToOpenWithRequest()));
}

void KSSendToMenu::populateKServiceSendToActions()
{
    const KService::List services = KMimeTypeTrader::self()->query("image/png");

    for (auto service: services) {
        QString name = service->name().replace('&', "&&");

        QAction *action = new QAction(QIcon::fromTheme(service->icon()), name, nullptr);
        action->setData(QVariant::fromValue(service));
        connect(action, &QAction::triggered, this, &KSSendToMenu::handleSendToKService);

        mMenu->addAction(action);
    }
}

#ifdef KIPI_FOUND
void KSSendToMenu::populateKipiSendToActions()
{
    mKipiInterface = new KSGKipiInterface(this);
    KIPI::PluginLoader *loader = new KIPI::PluginLoader;

    loader->setInterface(mKipiInterface);
    loader->init();

    KIPI::PluginLoader::PluginList pluginList = loader->pluginList();

    for (auto pluginInfo: pluginList) {
        if (!(pluginInfo->shouldLoad())) {
            continue;
        }

        KIPI::Plugin *plugin = pluginInfo->plugin();
        if (!(plugin)) {
            qWarning() << i18n("KIPI plugin from library %1 failed to load", pluginInfo->library());
            continue;
        }

        plugin->setup(&mDummyWidget);

        QList<QAction *> actions = plugin->actions();
        QSet<QAction *> exportActions;

        for (auto action: actions) {
            KIPI::Category category = plugin->category(action);
            if (category == KIPI::ExportPlugin) {
                exportActions += action;
            } else if (category == KIPI::ImagesPlugin && pluginInfo->library().contains("kipiplugin_sendimages")) {
                exportActions += action;
            }
        }

        for (auto action: exportActions) {
            mMenu->addAction(action);
        }
    }
}
#endif
