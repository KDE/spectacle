/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ExportMenu.h"
#include "CaptureWindow.h"
#include "SpectacleCore.h"
#include "WidgetWindowUtils.h"
#include "spectacle_gui_debug.h"
#include "settings.h"

#include <KApplicationTrader>
#include <KIO/ApplicationLauncherJob>
#include <kio_version.h>
#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenFileManagerWindowJob>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KStandardAction>
#include <KStandardShortcut>

#include <QJsonArray>
#include <QMimeDatabase>
#include <QPrintDialog>
#include <QPrinter>
#include <QTimer>
#include <QWindow>
#include <chrono>

using namespace std::chrono_literals;

class ExportMenuSingleton
{
public:
    ExportMenu self;
};

Q_GLOBAL_STATIC(ExportMenuSingleton, privateExportMenuSelf)

ExportMenu::ExportMenu(QWidget *parent)
    : SpectacleMenu(parent)
#ifdef PURPOSE_FOUND
    , mUpdatedImageAvailable(true)
    , mPurposeMenu(new Purpose::Menu)
#endif
{
    addAction(QIcon::fromTheme(QStringLiteral("document-open-folder")),
              i18n("Open Default Screenshots Folder"),
              this, &ExportMenu::openScreenshotsFolder);
    addAction(KStandardAction::print(this, &ExportMenu::openPrintDialog, this));

#ifdef PURPOSE_FOUND
    loadPurposeMenu();
    connect(ExportManager::instance(), &ExportManager::imageChanged, this, &ExportMenu::onImageChanged);
#endif

    addSeparator();
    getKServiceItems();
}

ExportMenu *ExportMenu::instance()
{
    return &privateExportMenuSelf->self;
}

void ExportMenu::onImageChanged()
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

        connect(action, &QAction::triggered, this, [this, service]() {
            auto captureWindow = qobject_cast<CaptureWindow *>(getWidgetTransientParent(this));
            if(captureWindow && !captureWindow->accept()) {
                return;
            }
            QUrl filename;
            if(ExportManager::instance()->isImageSavedNotInTemp()) {
                filename = Settings::self()->lastSaveLocation();
            } else {
                filename = ExportManager::instance()->getAutosaveFilename();
                SpectacleCore::instance()->syncExportImage();
                ExportManager::instance()->exportImage(ExportManager::Save, filename);
            }

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

    connect(openWith, &QAction::triggered, this, [this]() {
        auto captureWindow = qobject_cast<CaptureWindow *>(getWidgetTransientParent(this));
        if(captureWindow && !captureWindow->accept()) {
            return;
        }
        QUrl filename;
        if(ExportManager::instance()->isImageSavedNotInTemp()) {
            filename = Settings::self()->lastSaveLocation();
        } else {
            filename = ExportManager::instance()->getAutosaveFilename();
            SpectacleCore::instance()->syncExportImage();
            ExportManager::instance()->exportImage(ExportManager::Save, filename);
        }

        auto job = new KIO::ApplicationLauncherJob;
        job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, window()));
        job->setUrls({filename});
        job->start();
    });
    addAction(openWith);
}

#ifdef PURPOSE_FOUND
void ExportMenu::loadPurposeMenu()
{
    // attach the menu
    auto purposeMenu = mPurposeMenu.get();
    QAction *purposeMenuAction = addMenu(purposeMenu);
    purposeMenuAction->setObjectName("purposeMenuAction");
    purposeMenuAction->setText(i18n("Share"));
    purposeMenuAction->setIcon(QIcon::fromTheme(QStringLiteral("document-share")));

    // set up the callback signal
    connect(purposeMenu, &Purpose::Menu::finished, this, [this](const QJsonObject &output, int error, const QString &message) {
        if (error) {
            Q_EMIT imageShared(error, message);
        } else {
            Q_EMIT imageShared(error, output[QStringLiteral("url")].toString());
        }
    });

    // update available options based on the latest picture
    connect(purposeMenu, &QMenu::aboutToShow, this, [this]() {
        loadPurposeItems();
        setWidgetTransientParentToWidget(mPurposeMenu.get(), this);
    });
}

void ExportMenu::loadPurposeItems()
{
    if (!mUpdatedImageAvailable) {
        return;
    }

    // updated image available, we lazily load it now
    const QString dataUri = ExportManager::instance()->tempSave().toString();
    mUpdatedImageAvailable = false;

    auto mimeType = QMimeDatabase().mimeTypeForFile(dataUri).name();
    QJsonObject inputData = {
        {QStringLiteral("mimeType"), mimeType},
        {QStringLiteral("urls"), QJsonArray({dataUri})}
    };
    mPurposeMenu->model()->setInputData(inputData);
    mPurposeMenu->model()->setPluginType(QStringLiteral("Export"));
    mPurposeMenu->reload();
}
#endif

void ExportMenu::openScreenshotsFolder()
{
    auto job = new KIO::OpenUrlJob(Settings::defaultSaveLocation());
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
    job->start();
}

void ExportMenu::openPrintDialog()
{
    if (auto captureWindow = qobject_cast<CaptureWindow *>(getWidgetTransientParent(this))) {
        captureWindow->accept();
    }
    auto printer = new QPrinter(QPrinter::HighResolution);
    auto dialog = new QPrintDialog(printer);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    // properly set the transientparent chain
    setWidgetTransientParent(dialog, getWidgetTransientParent(this));

    connect(dialog, &QDialog::finished, dialog, [printer](int result){
        if (result == QDialog::Accepted) {
            ExportManager::instance()->doPrint(printer);
        }
        delete printer;
    });

    dialog->setVisible(true);
}

#include "moc_ExportMenu.cpp"
