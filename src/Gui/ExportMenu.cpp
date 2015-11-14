/*
 *  Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
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

#include "ExportMenu.h"

#include <QTimer>
#include <QList>
#include <QUrl>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

#include <KLocalizedString>
#include <KStandardShortcut>
#include <KService>
#include <KMimeTypeTrader>
#include <KRun>

#include "Config.h"
#ifdef KIPI_FOUND
#include <KIPI/Plugin>
#include <KIPI/PluginLoader>
#include "KipiInterface/KSGKipiInterface.h"
#endif

ExportMenu::ExportMenu(QWidget *parent) :
    QMenu(parent),
    mExportManager(ExportManager::instance())
{
    QTimer::singleShot(300, this, &ExportMenu::populateMenu);
}

void ExportMenu::populateMenu()
{
#ifdef KIPI_FOUND
    QMenu *kipiMenu = addMenu(QIcon::fromTheme(QStringLiteral("applications-internet")), i18n("Online Services"));
    kipiMenu->addAction(i18n("Please wait..."));
    QTimer::singleShot(750, [=]() { getKipiItems(kipiMenu); });

    addSeparator();
#endif
    getKServiceItems();
}

void ExportMenu::getKServiceItems()
{
    // populate all locally installed applications and services
    // which can handle images first

    const KService::List services = KMimeTypeTrader::self()->query(QStringLiteral("image/png"));

    Q_FOREACH (auto service, services) {
        QString name = service->name().replace('&', QLatin1String("&&"));
        QAction *action = new QAction(QIcon::fromTheme(service->icon()), name, nullptr);

        connect(action, &QAction::triggered, [=]() {
            QList<QUrl> whereIs({ mExportManager->tempSave() });
            KRun::runService(*service, whereIs, parentWidget(), true);
        });
        addAction(action);
    }

    // now let the user manually chose an application to open the
    // image with

    addSeparator();

    QAction *openWith = new QAction(this);
    openWith->setText(i18n("Other Application"));
    openWith->setIcon(QIcon::fromTheme(QStringLiteral("application-x-executable")));
    openWith->setShortcuts(KStandardShortcut::open());

    connect(openWith, &QAction::triggered, [=]() {
        QList<QUrl> whereIs({ mExportManager->tempSave() });
        KRun::displayOpenWithDialog(whereIs, parentWidget(), true);
    });
    addAction(openWith);
}

#ifdef KIPI_FOUND
void ExportMenu::getKipiItems(QMenu *menu)
{
    menu->clear();

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
            menu->addAction(action);
        }
    }
}
#endif
