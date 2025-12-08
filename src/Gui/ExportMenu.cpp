/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ExportMenu.h"
#include "CaptureWindow.h"
#include "OcrManager.h"
#include "SpectacleCore.h"
#include "WidgetWindowUtils.h"
#include "settings.h"

#include <KApplicationTrader>
#include <KIO/ApplicationLauncherJob>
#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenFileManagerWindowJob>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KStandardActions>
#include <KStandardShortcut>
#include <kio_version.h>

#include <QJsonArray>
#include <QMimeDatabase>
#include <QPrintDialog>
#include <QPrinter>
#include <QTimer>
#include <QWindow>
#include <chrono>

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

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
    setTitle(i18nc("@title:menu", "Export"));
    setIcon(QIcon::fromTheme(u"document-share"_s));
    addAction(QIcon::fromTheme(u"document-open-folder"_s),
              i18n("Open Default Screenshots Folder"),
              this, &ExportMenu::openScreenshotsFolder);
    addAction(KStandardActions::print(this, &ExportMenu::openPrintDialog, this));

    createOcrLanguageSubmenu();

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

    const KService::List services = KApplicationTrader::queryByMimeType(u"image/png"_s);

    for (auto service : services) {
        const QString name = service->name().replace('&'_L1, "&&"_L1);
        QAction *action = new QAction(QIcon::fromTheme(service->icon()), name, this);

        connect(action, &QAction::triggered, this, [this, service]() {
            auto captureWindow = qobject_cast<CaptureWindow *>(getWidgetTransientParent(this));
            if(captureWindow && !captureWindow->accept()) {
                return;
            }
            QUrl filename;
            if(ExportManager::instance()->isImageSavedNotInTemp()) {
                filename = Settings::self()->lastImageSaveLocation();
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

    QAction *openWith = new QAction(i18nc("@action:button open screenshot in other application", "Other Application…"), this);
    openWith->setShortcuts(KStandardShortcut::open());

    connect(openWith, &QAction::triggered, this, [this]() {
        auto captureWindow = qobject_cast<CaptureWindow *>(getWidgetTransientParent(this));
        if(captureWindow && !captureWindow->accept()) {
            return;
        }
        QUrl filename;
        if(ExportManager::instance()->isImageSavedNotInTemp()) {
            filename = Settings::self()->lastImageSaveLocation();
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
    purposeMenuAction->setIcon(QIcon::fromTheme(u"document-share"_s));

    // set up the callback signal
    connect(purposeMenu, &Purpose::Menu::finished, this, [this](const QJsonObject &output, int error, const QString &message) {
        if (error) {
            Q_EMIT imageShared(error, message);
        } else {
            Q_EMIT imageShared(error, output[u"url"_s].toString());
        }
    });

    // update available options based on the latest picture
    connect(purposeMenu, &QMenu::aboutToShow, this, [this]() {
        loadPurposeItems();
        setWidgetTransientParentToWidget(mPurposeMenu.get(), this);
    });

    connect(purposeMenu, &Purpose::Menu::aboutToShare, this, [this]() {
        // Before starting the share operation, accept to avoid sharing unwanted portions of the screen
        auto captureWindow = qobject_cast<CaptureWindow *>(getWidgetTransientParent(this));
        if (captureWindow && !captureWindow->accept()) {
            return;
        }
        const QString dataUri = ExportManager::instance()->tempSave().toString();
        auto mimeType = QMimeDatabase().mimeTypeForFile(dataUri).name();
        QJsonObject inputData = {{u"mimeType"_s, mimeType}, {u"urls"_s, QJsonArray({dataUri})}};
        mPurposeMenu->model()->setInputData(inputData);
        mPurposeMenu->model()->setPluginType(u"Export"_s);
    });
}

void ExportMenu::loadPurposeItems()
{
    if (!mUpdatedImageAvailable) {
        return;
    }

    // updated image available, we lazily load it now
    ExportManager::instance()->updateTimestamp();
    const QString dataUri = ExportManager::instance()->tempSave().toString();
    mUpdatedImageAvailable = false;

    auto mimeType = QMimeDatabase().mimeTypeForFile(dataUri).name();
    QJsonObject inputData = {
        {u"mimeType"_s, mimeType},
        {u"urls"_s, QJsonArray({dataUri})}
    };
    mPurposeMenu->model()->setInputData(inputData);
    mPurposeMenu->model()->setPluginType(u"Export"_s);
    mPurposeMenu->reload();
}
#endif

void ExportMenu::openScreenshotsFolder()
{
    auto job = new KIO::OpenUrlJob(Settings::imageSaveLocation());
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

void ExportMenu::createOcrLanguageSubmenu()
{
    Q_ASSERT(!m_ocrLanguageMenu);

    auto ocrManager = OcrManager::instance();

    if (!ocrManager || !ocrManager->isAvailable()) {
        return;
    }

    m_ocrLanguageMenu = addMenu(i18nc("@action:menu", "Extract Text by Language"));
    m_ocrLanguageMenu->setIcon(QIcon::fromTheme(u"document-scan"_s));

    // Keep the submenu in sync with OCR status changes
    if (ocrManager) {
        connect(ocrManager, &OcrManager::statusChanged, this, &ExportMenu::buildOcrLanguageSubmenu);
    }

    if (auto settings = Settings::self()) {
        connect(settings, &Settings::ocrLanguagesChanged, this, &ExportMenu::buildOcrLanguageSubmenu);
    }

    connect(m_ocrLanguageMenu, &QMenu::aboutToShow, this, &ExportMenu::buildOcrLanguageSubmenu);

    buildOcrLanguageSubmenu();
}

void ExportMenu::buildOcrLanguageSubmenu()
{
    if (!m_ocrLanguageMenu) {
        return;
    }

    auto *menuAction = m_ocrLanguageMenu->menuAction();
    const auto setMenuEnabled = [this, menuAction](bool enabled) {
        m_ocrLanguageMenu->setEnabled(enabled);
        if (menuAction) {
            menuAction->setEnabled(enabled);
        }
    };

    m_ocrLanguageMenu->clear();

    auto ocrManager = OcrManager::instance();

    if (!ocrManager) {
        QAction *action = m_ocrLanguageMenu->addAction(i18n("OCR engine is not available."));
        action->setEnabled(false);
        setMenuEnabled(false);
        return;
    }

    const bool initializationFailed = ocrManager->status() == OcrManager::OcrStatus::Error;
    if (!ocrManager->isAvailable()) {
        QAction *action = m_ocrLanguageMenu->addAction(initializationFailed ? i18n("OCR is not available. Please install Tesseract OCR.")
                                                                            : i18n("OCR engine is initializing…"));
        action->setEnabled(false);
        setMenuEnabled(false);
        return;
    }

    const bool busy = ocrManager->status() == OcrManager::OcrStatus::Processing;
    const QMap<QString, QString> languages = ocrManager->availableLanguagesWithNames();

    if (languages.isEmpty()) {
        QAction *action = m_ocrLanguageMenu->addAction(i18n("No OCR language data available."));
        action->setEnabled(false);
        setMenuEnabled(false);
        return;
    }

    setMenuEnabled(!busy);

    for (auto it = languages.cbegin(); it != languages.cend(); ++it) {
        const QString &code = it.key();

        if (code == u"osd"_s) {
            continue;
        }

        QAction *languageAction = m_ocrLanguageMenu->addAction(it.value());
        languageAction->setEnabled(!busy);

        connect(languageAction, &QAction::triggered, this, [this, code]() {
            triggerExtraction(code);
        });
    }
}

void ExportMenu::triggerExtraction(const QString &languageCode)
{
    SpectacleCore::instance()->startOcrExtraction(languageCode);
}

#include "moc_ExportMenu.cpp"
