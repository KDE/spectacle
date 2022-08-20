/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ExportMenu.h"
#include "spectacle_gui_debug.h"

#include <KApplicationTrader>
#include <KIO/ApplicationLauncherJob>
#include <kio_version.h>
#if KIO_VERSION >= QT_VERSION_CHECK(5, 98, 0)
#include <KIO/JobUiDelegateFactory>
#else
#include <KIO/JobUiDelegate>
#endif
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KStandardShortcut>

#include <QJsonArray>
#include <QTimer>
#include <chrono>

using namespace std::chrono_literals;

ExportMenu::ExportMenu(QWidget *parent)
    : QMenu(parent)
#ifdef PURPOSE_FOUND
    , mUpdatedImageAvailable(false)
    , mPurposeMenu(new Purpose::Menu(this))
#endif
    , mExportManager(ExportManager::instance())
{
    QTimer::singleShot(300ms, this, &ExportMenu::populateMenu);
}

void ExportMenu::populateMenu()
{
#ifdef PURPOSE_FOUND
    loadPurposeMenu();
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
        const QString name = service->name().replace(QLatin1Char('&'), QLatin1String("&&"));
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

    QAction *openWith = new QAction(i18n("Other Application..."), this);
    openWith->setShortcuts(KStandardShortcut::open());

    connect(openWith, &QAction::triggered, this, [=]() {
        const QUrl filename = mExportManager->getAutosaveFilename();
        mExportManager->doSave(filename);
        auto job = new KIO::ApplicationLauncherJob;
#if KIO_VERSION >= QT_VERSION_CHECK(5, 98, 0)
        job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, window()));
#else
        job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, window()));
#endif
        job->setUrls({filename});
        job->start();
    });
    addAction(openWith);
}

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
            Q_EMIT imageShared(error, message);
        } else {
            Q_EMIT imageShared(error, output[QStringLiteral("url")].toString());
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
    const QString dataUri = ExportManager::instance()->tempSave().toString();
    mUpdatedImageAvailable = false;

    mPurposeMenu->model()->setInputData(
        QJsonObject{{QStringLiteral("mimeType"), QStringLiteral("image/png")}, {QStringLiteral("urls"), QJsonArray({dataUri})}});
    mPurposeMenu->model()->setPluginType(QStringLiteral("Export"));
    mPurposeMenu->reload();
}
#endif
