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

#include "SpectacleCore.h"
#include "spectacle_core_debug.h"

#include "Config.h"

#include <KGlobalAccel>
#include <KLocalizedString>
#include <KMessageBox>
#include <KMessageWidget>
#include <KNotification>
#include <KRun>
#include <KWindowSystem>

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QDrag>
#include <QKeySequence>
#include <QMimeData>
#include <QProcess>
#include <QTimer>

SpectacleCore::SpectacleCore(StartMode theStartMode,
                             Spectacle::CaptureMode theCaptureMode,
                             QString &theSaveFileName,
                             qint64 theDelayMsec,
                             bool theNotifyOnGrab,
                             bool theCopyToClipboard,
                             QObject *parent) :
    QObject(parent),
    mStartMode(theStartMode),
    mNotify(theNotifyOnGrab),
    mPlatform(loadPlatform()),
    mMainWindow(nullptr),
    mIsGuiInited(false),
    mCopySaveLocationToClipboard(theCopyToClipboard)
{
    auto lConfig = KSharedConfig::openConfig(QStringLiteral("spectaclerc"));
    KConfigGroup lGuiConfig(lConfig, "GuiConfig");

    if (!(theSaveFileName.isEmpty() || theSaveFileName.isNull())) {
        if (QDir::isRelativePath(theSaveFileName)) {
            theSaveFileName = QDir::current().absoluteFilePath(theSaveFileName);
        }
        setFilename(theSaveFileName);
    }

    // essential connections
    connect(this, &SpectacleCore::errorMessage, this, &SpectacleCore::showErrorMessage);
    connect(mPlatform.get(), &Platform::newScreenshotTaken, this, &SpectacleCore::screenshotUpdated);
    connect(mPlatform.get(), &Platform::newScreenshotFailed, this, &SpectacleCore::screenshotFailed);

    auto lImmediateAvailable = mPlatform->supportedShutterModes().testFlag(Platform::ShutterMode::Immediate);
    auto lOnClickAvailable = mPlatform->supportedShutterModes().testFlag(Platform::ShutterMode::OnClick);
    if ((!lOnClickAvailable) && (theDelayMsec < 0)) {
        theDelayMsec = 0;
    }

    // reset last region if it should not be remembered across restarts
    auto lSpectacleConfig = SpectacleConfig::instance();
    if(!lSpectacleConfig->alwaysRememberRegion()) {
        lSpectacleConfig->setCropRegion(QRect());
    }

    // set up the export manager
    auto lExportManager = ExportManager::instance();
    lExportManager->setCaptureMode(theCaptureMode);
    connect(lExportManager, &ExportManager::errorMessage, this, &SpectacleCore::showErrorMessage);
    connect(lExportManager, &ExportManager::imageSaved, this, &SpectacleCore::doCopyPath);
    connect(lExportManager, &ExportManager::forceNotify, this, &SpectacleCore::doNotify);
    connect(mPlatform.get(), &Platform::windowTitleChanged, lExportManager, &ExportManager::setWindowTitle);

    switch (theStartMode) {
    case StartMode::DBus:
        break;
    case StartMode::Background: {
            auto lMsec = (KWindowSystem::compositingActive() ? 200 : 50) + theDelayMsec;
            auto lShutterMode = lImmediateAvailable ? Platform::ShutterMode::Immediate : Platform::ShutterMode::OnClick;
            auto lIncludePointer = lGuiConfig.readEntry("includePointer", true);
            auto lIncludeDecorations = lGuiConfig.readEntry("includeDecorations", true);
            const Platform::GrabMode lCaptureMode = toPlatformGrabMode(theCaptureMode);
            QTimer::singleShot(lMsec, this, [ this, lCaptureMode, lShutterMode, lIncludePointer, lIncludeDecorations ]() {
                mPlatform->doGrab(lShutterMode, lCaptureMode, lIncludePointer, lIncludeDecorations);
            });
        }
        break;
    case StartMode::Gui:
        initGui(lGuiConfig.readEntry("includePointer", true), lGuiConfig.readEntry("includeDecorations", true));
        break;
    }
    setUpShortcuts();
}

void SpectacleCore::setUpShortcuts()
{
    SpectacleConfig* config = SpectacleConfig::instance();

    QAction* openAction = config->shortCutActions->action(QStringLiteral("_launch"));
    KGlobalAccel::self()->setGlobalShortcut(openAction, Qt::Key_Print);

    QAction* fullScreenAction = config->shortCutActions->action(QStringLiteral("FullScreenScreenShot"));
    KGlobalAccel::self()->setGlobalShortcut(fullScreenAction, Qt::SHIFT + Qt::Key_Print);

    QAction* activeWindowAction = config->shortCutActions->action(QStringLiteral("ActiveWindowScreenShot"));
    KGlobalAccel::self()->setGlobalShortcut(activeWindowAction, Qt::META + Qt::Key_Print);

    QAction* regionAction = config->shortCutActions->action(QStringLiteral("RectangularRegionScreenShot"));
    KGlobalAccel::self()->setGlobalShortcut(regionAction, Qt::META + Qt::SHIFT + Qt::Key_Print);

    QAction* currentScreenAction = config->shortCutActions->action(QStringLiteral("CurrentMonitorScreenShot"));
    KGlobalAccel::self()->setGlobalShortcut(currentScreenAction, QList<QKeySequence>());
}

QString SpectacleCore::filename() const
{
    return mFileNameString;
}

void SpectacleCore::setFilename(const QString &filename)
{
    mFileNameString = filename;
    mFileNameUrl = QUrl::fromUserInput(filename);
}

// Slots

void SpectacleCore::dbusStartAgent()
{
    qApp->setQuitOnLastWindowClosed(true);

    auto lConfig = KSharedConfig::openConfig(QStringLiteral("spectaclerc"));
    KConfigGroup lGuiConfig(lConfig, "GuiConfig");
    auto lIncludePointer = lGuiConfig.readEntry("includePointer", true);
    auto lIncludeDecorations = lGuiConfig.readEntry("includeDecorations", true);

    if (!(mStartMode == StartMode::Gui)) {
        mStartMode = StartMode::Gui;
        initGui(lIncludePointer, lIncludeDecorations);
    } else {
        using Actions = SpectacleConfig::PrintKeyActionRunning;
        switch (SpectacleConfig::instance()->printKeyActionRunning()) {
            case Actions::TakeNewScreenshot: {
                auto lShutterMode = mPlatform->supportedShutterModes().testFlag(Platform::ShutterMode::Immediate) ? Platform::ShutterMode::Immediate : Platform::ShutterMode::OnClick;
                auto lGrabMode = toPlatformGrabMode(ExportManager::instance()->captureMode());
                QTimer::singleShot(KWindowSystem::compositingActive() ? 200 : 50, this, [this, lShutterMode, lGrabMode, lIncludePointer, lIncludeDecorations]() {
                    mPlatform->doGrab(lShutterMode, lGrabMode, lIncludePointer, lIncludeDecorations);
                });
                break;
            }
            case Actions::FocusWindow:
                if (mMainWindow->isMinimized()) {
                    mMainWindow->setWindowState(mMainWindow->windowState() & ~Qt::WindowMinimized);
                }
                mMainWindow->activateWindow();
                break;
            case Actions::StartNewInstance:
                QProcess newInstance;
                newInstance.setProgram(QStringLiteral("spectacle"));
                newInstance.startDetached();
                break;
        }
    }
}

void SpectacleCore::takeNewScreenshot(Spectacle::CaptureMode theCaptureMode,
                                      int  theTimeout,
                                      bool theIncludePointer,
                                      bool theIncludeDecorations)
{
    ExportManager::instance()->setCaptureMode(theCaptureMode);
    auto lGrabMode = toPlatformGrabMode(theCaptureMode);

    if (theTimeout < 0) {
        mPlatform->doGrab(Platform::ShutterMode::OnClick, lGrabMode, theIncludePointer, theIncludeDecorations);
        return;
    }

    // when compositing is enabled, we need to give it enough time for the window
    // to disappear and all the effects are complete before we take the shot. there's
    // no way of knowing how long the disappearing effects take, but as per default
    // settings (and unless the user has set an extremely slow effect), 200
    // milliseconds is a good amount of wait time.

    auto lMsec = KWindowSystem::compositingActive() ? 200 : 50;
    QTimer::singleShot(theTimeout + lMsec, this, [this, lGrabMode, theIncludePointer, theIncludeDecorations]() {
        mPlatform->doGrab(Platform::ShutterMode::Immediate, lGrabMode, theIncludePointer, theIncludeDecorations);
    });
}

void SpectacleCore::showErrorMessage(const QString &theErrString)
{
    qCDebug(SPECTACLE_CORE_LOG) << "ERROR: " << theErrString;

    if (mStartMode == StartMode::Gui) {
        KMessageBox::error(nullptr, theErrString);
    }
}

void SpectacleCore::screenshotUpdated(const QPixmap &thePixmap)
{
    auto lExportManager = ExportManager::instance();

    // if we were running in rectangular crop mode, now would be
    // the time to further process the image

    if (lExportManager->captureMode() == Spectacle::CaptureMode::RectangularRegion) {
        if(!mQuickEditor) {
            mQuickEditor = std::make_unique<QuickEditor>(thePixmap);
            connect(mQuickEditor.get(), &QuickEditor::grabDone, this, &SpectacleCore::screenshotUpdated);
            connect(mQuickEditor.get(), &QuickEditor::grabCancelled, this, &SpectacleCore::screenshotFailed);
            mQuickEditor->show();
            return;
        } else {
            mQuickEditor->hide();
            mQuickEditor.reset(nullptr);
        }
    }

    lExportManager->setPixmap(thePixmap);
    lExportManager->updatePixmapTimestamp();

    switch (mStartMode) {
    case StartMode::Background:
    case StartMode::DBus:
        {
            if (mNotify) {
                connect(lExportManager, &ExportManager::imageSaved, this, &SpectacleCore::doNotify);
            }

            if (mCopySaveLocationToClipboard) {
                lExportManager->doCopyToClipboard(mNotify);
            } else {
                QUrl lSavePath = (mStartMode == StartMode::Background && mFileNameUrl.isValid() && mFileNameUrl.isLocalFile()) ?
                    mFileNameUrl : QUrl();
                lExportManager->doSave(lSavePath);
            }

            // if we notify, we emit allDone only if the user either dismissed the notification or pressed
            // the "Open" button, otherwise the app closes before it can react to it.
            if (!mNotify) {
                emit allDone();
            }
        }
        break;
    case StartMode::Gui:
        mMainWindow->setScreenshotAndShow(thePixmap);

        bool autoSaveImage = SpectacleConfig::instance()->autoSaveImage();
        bool copyImageToClipboard = SpectacleConfig::instance()->copyImageToClipboard();

        if (autoSaveImage && copyImageToClipboard) {
            lExportManager->doSaveAndCopy();
        } else if (autoSaveImage) {
            lExportManager->doSave();
        } else if (copyImageToClipboard) {
            lExportManager->doCopyToClipboard(false);
            mMainWindow->showInlineMessage(i18n("The screenshot has been copied to the clipboard."),
                                            KMessageWidget::Information);
        }
    }
}

void SpectacleCore::screenshotFailed()
{
    if (ExportManager::instance()->captureMode() == Spectacle::CaptureMode::RectangularRegion && mQuickEditor) {
        mQuickEditor->hide();
        mQuickEditor.reset(nullptr);
    }

    switch (mStartMode) {
    case StartMode::Background:
        showErrorMessage(i18n("Screenshot capture canceled or failed"));
        emit allDone();
        return;
    case StartMode::DBus:
        emit grabFailed();
        emit allDone();
        return;
    case StartMode::Gui:
        mMainWindow->setScreenshotAndShow(QPixmap());
    }
}

void SpectacleCore::doNotify(const QUrl &theSavedAt)
{
    KNotification *lNotify = new KNotification(QStringLiteral("newScreenshotSaved"));

    switch(ExportManager::instance()->captureMode()) {
    case Spectacle::CaptureMode::AllScreens:
        lNotify->setTitle(i18nc("The entire screen area was captured, heading", "Full Screen Captured"));
        break;
    case Spectacle::CaptureMode::CurrentScreen:
        lNotify->setTitle(i18nc("The current screen was captured, heading", "Current Screen Captured"));
        break;
    case Spectacle::CaptureMode::ActiveWindow:
        lNotify->setTitle(i18nc("The active window was captured, heading", "Active Window Captured"));
        break;
    case Spectacle::CaptureMode::WindowUnderCursor:
    case Spectacle::CaptureMode::TransientWithParent:
        lNotify->setTitle(i18nc("The window under the mouse was captured, heading", "Window Under Cursor Captured"));
        break;
    case Spectacle::CaptureMode::RectangularRegion:
        lNotify->setTitle(i18nc("A rectangular region was captured, heading", "Rectangular Region Captured"));
        break;
    case Spectacle::CaptureMode::InvalidChoice:
        break;
    }

    // a speaking message is prettier than a URL, special case for copy to clipboard and the default pictures location
    const QString &lSavePath = theSavedAt.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).path();
    if (mCopySaveLocationToClipboard) {
        lNotify->setText(i18n("A screenshot was saved to your clipboard."));
    } else if (lSavePath == QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)) {
        lNotify->setText(i18nc("Placeholder is filename", "A screenshot was saved as '%1' to your Pictures folder.", theSavedAt.fileName()));
    } else {
        lNotify->setText(i18n("A screenshot was saved as '%1' to '%2'.", theSavedAt.fileName(), lSavePath));
    }

    if (!mCopySaveLocationToClipboard) {
        lNotify->setUrls({theSavedAt});
        lNotify->setDefaultAction(i18nc("Open the screenshot we just saved", "Open"));
        connect(lNotify, QOverload<uint>::of(&KNotification::activated), this, [this, theSavedAt](uint index) {
            if (index == 0) {
                new KRun(theSavedAt, nullptr);
                QTimer::singleShot(250, this, [this] {
                    if (mStartMode != StartMode::Gui) {
                        emit allDone();
                    }
                });
            }
        });
    }

    connect(lNotify, &QObject::destroyed, this, [this] {
        if (mStartMode != StartMode::Gui) {
            emit allDone();
        }
    });

    lNotify->sendEvent();
}

void SpectacleCore::doCopyPath(const QUrl &savedAt)
{
    if (SpectacleConfig::instance()->copySaveLocationToClipboard()) {
        qApp->clipboard()->setText(savedAt.toLocalFile());
    }
}

void SpectacleCore::doStartDragAndDrop()
{
    auto lExportManager = ExportManager::instance();
    QUrl lTempFile = lExportManager->tempSave();
    if (!lTempFile.isValid()) {
        return;
    }

    auto lMimeData = new QMimeData;
    lMimeData->setUrls(QList<QUrl> { lTempFile });
    lMimeData->setData(QStringLiteral("application/x-kde-suggestedfilename"), QFile::encodeName(lTempFile.fileName()));

    auto lDragHandler = new QDrag(this);
    lDragHandler->setMimeData(lMimeData);
    lDragHandler->setPixmap(lExportManager->pixmap().scaled(256, 256, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    lDragHandler->exec(Qt::CopyAction);
}

// Private

Platform::GrabMode SpectacleCore::toPlatformGrabMode(Spectacle::CaptureMode theCaptureMode)
{
    switch(theCaptureMode) {
    case Spectacle::CaptureMode::InvalidChoice:
        return Platform::GrabMode::InvalidChoice;
    case Spectacle::CaptureMode::AllScreens:
    case Spectacle::CaptureMode::RectangularRegion:
        return Platform::GrabMode::AllScreens;
    case Spectacle::CaptureMode::TransientWithParent:
        return Platform::GrabMode::TransientWithParent;
    case Spectacle::CaptureMode::CurrentScreen:
        return Platform::GrabMode::CurrentScreen;
    case Spectacle::CaptureMode::ActiveWindow:
        return Platform::GrabMode::ActiveWindow;
    case Spectacle::CaptureMode::WindowUnderCursor:
        return Platform::GrabMode::WindowUnderCursor;
    }
    return Platform::GrabMode::InvalidChoice;
}

void SpectacleCore::initGui(bool theIncludePointer, bool theIncludeDecorations)
{
    if (!mIsGuiInited) {
        mMainWindow = std::make_unique<KSMainWindow>(mPlatform->supportedGrabModes(), mPlatform->supportedShutterModes());

        connect(mMainWindow.get(), &KSMainWindow::newScreenshotRequest, this, &SpectacleCore::takeNewScreenshot);
        connect(mMainWindow.get(), &KSMainWindow::dragAndDropRequest, this, &SpectacleCore::doStartDragAndDrop);

        mIsGuiInited = true;

        auto lShutterMode = mPlatform->supportedShutterModes().testFlag(Platform::ShutterMode::Immediate) ? Platform::ShutterMode::Immediate : Platform::ShutterMode::OnClick;
        auto lGrabMode = toPlatformGrabMode(ExportManager::instance()->captureMode());

        QTimer::singleShot(0, this, [this, lShutterMode, lGrabMode, theIncludePointer, theIncludeDecorations]() {
            mPlatform->doGrab(lShutterMode, lGrabMode, theIncludePointer, theIncludeDecorations);
        });
    }
}
