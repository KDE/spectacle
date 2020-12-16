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
#include "spectacle_gui_debug.h"

#include <KLocalizedString>
#include <KApplicationTrader>
#include <KIO/ApplicationLauncherJob>
#include <KNotificationJobUiDelegate>
#include <KStandardShortcut>
#ifdef KIPI_FOUND
#include <KIPI/Plugin>
#endif

#include <QJsonArray>
#include <QTimer>

ExportMenu::ExportMenu(QWidget *parent) :
    QMenu(parent),
#ifdef PURPOSE_FOUND
    mUpdatedImageAvailable(false),
    mPurposeMenu(new Purpose::Menu(this)),
#endif
    mExportManager(ExportManager::instance())
{
    QTimer::singleShot(300, this, &ExportMenu::populateMenu);
}

void ExportMenu::populateMenu()
{
#ifdef PURPOSE_FOUND
    loadPurposeMenu();
#endif

#ifdef KIPI_FOUND
    mKipiMenu = addMenu(i18n("More Online Services"));
    mKipiMenu->addAction(i18n("Please wait..."));
    mKipiMenuLoaded = false;

    connect(mKipiMenu, &QMenu::aboutToShow, this, &ExportMenu::loadKipiItems);
#endif

    addSeparator();
    getKServiceItems();
}


void ExportMenu::imageUpdated()
{
#ifdef PURPOSE_FOUND
    // mark cached image as stale
    mUpdatedImageAvailable = true;
    mPurposeMenu->clear();
#endif
}


void ExportMenu::getKServiceItems()
{
    // populate all locally installed applications and services
    // which can handle images first

    const KService::List services = KApplicationTrader::queryByMimeType(QStringLiteral("image/png"));

    for (auto service : services) {
        QString name = service->name().replace(QLatin1Char('&'), QLatin1String("&&"));
        QAction *action = new QAction(QIcon::fromTheme(service->icon()), name, this);

        connect(action, &QAction::triggered, this, [=]() {
            const QUrl filename = mExportManager->getAutosaveFilename();
            mExportManager->doSave(filename);

            auto *job = new KIO::ApplicationLauncherJob(service);
            auto *delegate = new KNotificationJobUiDelegate;
            delegate->setAutoErrorHandlingEnabled(true);
            job->setUiDelegate(delegate);

            job->setUrls({filename});
            job->start();
        });
        addAction(action);
    }

    // now let the user manually chose an application to open the
    // image with

    addSeparator();

    QAction *openWith = new QAction(this);
    openWith->setText(i18n("Other Application..."));
    openWith->setShortcuts(KStandardShortcut::open());

    connect(openWith, &QAction::triggered, this, [=]() {
        const QUrl filename = mExportManager->getAutosaveFilename();
        mExportManager->doSave(filename);
        QList<QUrl> whereIs({ filename });
        auto job = new KIO::ApplicationLauncherJob;
        job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));
        job->setUrls({filename});
        job->start();
    });
    addAction(openWith);
}

#ifdef KIPI_FOUND
void ExportMenu::loadKipiItems()
{
    if (!mKipiMenuLoaded) {
        QTimer::singleShot(500, this, &ExportMenu::getKipiItems);
        mKipiMenuLoaded = true;
    }
}

void ExportMenu::getKipiItems()
{
    mKipiMenu->clear();

    mKipiInterface = new KSGKipiInterface(this);
    KIPI::PluginLoader *loader = new KIPI::PluginLoader;

    loader->setInterface(mKipiInterface);
    loader->init();

    KIPI::PluginLoader::PluginList pluginList = loader->pluginList();

    for (const auto &pluginInfo : qAsConst(pluginList)) {
        if (!(pluginInfo->shouldLoad())) {
            continue;
        }

        KIPI::Plugin *plugin = pluginInfo->plugin();
        if (!(plugin)) {
            qCWarning(SPECTACLE_GUI_LOG) << i18n("KIPI plugin from library %1 failed to load", pluginInfo->library());
            continue;
        }

        plugin->setup(&mDummyWidget);

        const QList<QAction *> actions = plugin->actions();
        QSet<QAction *> exportActions;

        for (auto action : actions) {
            KIPI::Category category = plugin->category(action);
            if (category == KIPI::ExportPlugin) {
                exportActions += action;
            } else if (category == KIPI::ImagesPlugin && pluginInfo->library().contains(QLatin1String("kipiplugin_sendimages"))) {
                exportActions += action;
            }
        }

        for (auto action : qAsConst(exportActions)) {
            mKipiMenu->addAction(action);
        }
    }

    // If there are no export actions, then perhaps the kipi-plugins package is not installed.
    if (mKipiMenu->isEmpty()) {
        mKipiMenu->addAction(i18n("No KIPI plugins available"))->setEnabled(false);
    }
    delete loader;
}
#endif

#ifdef PURPOSE_FOUND
void ExportMenu::loadPurposeMenu()
{
    // attach the menu
    QAction *purposeMenu = addMenu(mPurposeMenu);
    purposeMenu->setText(i18n("Share"));
    purposeMenu->setIcon(QIcon::fromTheme(QStringLiteral("document-share")));

    // set up the callback signal
    connect(mPurposeMenu, &Purpose::Menu::finished, this, [this](const QJsonObject &output, int error, const QString &message) {
        if (error) {
            emit imageShared(true, message);
        } else {
            emit imageShared(false, output[QStringLiteral("url")].toString());
        }
    });

    // update available options based on the latest picture
    connect(mPurposeMenu, &QMenu::aboutToShow, this, &ExportMenu::loadPurposeItems);
}

void ExportMenu::loadPurposeItems()
{
    if (!mUpdatedImageAvailable) {
        return;
    }

    // updated image available, we lazily load it now
    QString dataUri = ExportManager::instance()->tempSave().toString();
    mUpdatedImageAvailable = false;

    mPurposeMenu->model()->setInputData(QJsonObject {
        { QStringLiteral("mimeType"), QStringLiteral("image/png") },
        { QStringLiteral("urls"), QJsonArray({ dataUri }) }
    });
    mPurposeMenu->model()->setPluginType(QStringLiteral("Export"));
    mPurposeMenu->reload();
}
#endif
