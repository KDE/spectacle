/*
 *  SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SpectacleCore.h"
#include "spectacle_core_debug.h"

#include "Config.h"
#include "ShortcutActions.h"
#include "settings.h"

#include <KGlobalAccel>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KMessageBox>
#include <KNotification>
#include <KUrlMimeData>
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/registry.h>
#include <KWindowSystem>

#include <QApplication>
#include <QClipboard>
#include <QCommandLineParser>
#include <QDir>
#include <QDrag>
#include <QKeySequence>
#include <QMimeData>
#include <QProcess>
#include <QScopedPointer>
#include <QScreen>
#include <QTimer>

SpectacleCore::SpectacleCore(QObject *parent)
    : QObject(parent)
{
}

void SpectacleCore::init()
{
    mPlatform = loadPlatform();

    // essential connections
    connect(this, &SpectacleCore::errorMessage, this, &SpectacleCore::showErrorMessage);
    connect(mPlatform.get(), &Platform::newScreenshotTaken, this, &SpectacleCore::screenshotUpdated);
    connect(mPlatform.get(), &Platform::newScreensScreenshotTaken, this, &SpectacleCore::screenshotsUpdated);
    connect(mPlatform.get(), &Platform::newScreenshotFailed, this, &SpectacleCore::screenshotFailed);

    // set up the export manager
    auto lExportManager = ExportManager::instance();
    connect(lExportManager, &ExportManager::errorMessage, this, &SpectacleCore::showErrorMessage);
    connect(lExportManager, &ExportManager::imageSaved, this, &SpectacleCore::doCopyPath);
    connect(lExportManager, &ExportManager::forceNotify, this, &SpectacleCore::doNotify);
    connect(mPlatform.get(), &Platform::windowTitleChanged, lExportManager, &ExportManager::setWindowTitle);

    // Needed so the QuickEditor can go fullscreen on wayland
    if (KWindowSystem::isPlatformWayland()) {
        using namespace KWayland::Client;
        ConnectionThread *connection = ConnectionThread::fromApplication(this);
        if (!connection) {
            return;
        }
        Registry *registry = new Registry(this);
        registry->create(connection);
        connect(registry, &Registry::plasmaShellAnnounced, this, [this, registry](quint32 name, quint32 version) {
            mWaylandPlasmashell = registry->createPlasmaShell(name, version, this);
        });
        registry->setup();
        connection->roundtrip();
    }
    setUpShortcuts();
}

void SpectacleCore::onActivateRequested(QStringList arguments, const QString & /*workingDirectory */)
{
    // QCommandLineParser expects the first argument to be the executable name
    // In the current version it just strips it away
    arguments.prepend(qApp->applicationFilePath());

    // We can't re-use QCommandLineParser instances, it preserves earlier parsed values
    QScopedPointer<QCommandLineParser> parser(new QCommandLineParser);
    populateCommandLineParser(parser.data());
    parser->parse(arguments);

    mStartMode = SpectacleCore::StartMode::Gui;
    mExistingLoaded = false;
    mNotify = true;
    qint64 lDelayMsec = 0;

    // are we ask to run in background or dbus mode?
    if (parser->isSet(QStringLiteral("background"))) {
        mStartMode = SpectacleCore::StartMode::Background;
    } else if (parser->isSet(QStringLiteral("dbus"))) {
        mStartMode = SpectacleCore::StartMode::DBus;
    }

    mEditExisting = parser->isSet(QStringLiteral("edit-existing"));
    if (mEditExisting) {
        QString lExistingFileName = parser->value(QStringLiteral("edit-existing"));
        if (!(lExistingFileName.isEmpty() || lExistingFileName.isNull())) {
            if (QDir::isRelativePath(lExistingFileName)) {
                lExistingFileName = QDir::current().absoluteFilePath(lExistingFileName);
            }
            setFilename(lExistingFileName);
            mSaveToOutput = true;
        }
    }

    auto lOnClickAvailable = mPlatform->supportedShutterModes().testFlag(Platform::ShutterMode::OnClick);
    if ((!lOnClickAvailable) && (lDelayMsec < 0)) {
        lDelayMsec = 0;
    }

    // reset last region if it should not be remembered across restarts
    if (!(Settings::rememberLastRectangularRegion() == Settings::EnumRememberLastRectangularRegion::Always)) {
        Settings::setCropRegion({0, 0, 0, 0});
    }

    Spectacle::CaptureMode lCaptureMode = Spectacle::CaptureMode::AllScreens;
    // extract the capture mode
    if (parser->isSet(QStringLiteral("fullscreen"))) {
        lCaptureMode = Spectacle::CaptureMode::AllScreens;
    } else if (parser->isSet(QStringLiteral("current"))) {
        lCaptureMode = Spectacle::CaptureMode::CurrentScreen;
    } else if (parser->isSet(QStringLiteral("activewindow"))) {
        lCaptureMode = Spectacle::CaptureMode::ActiveWindow;
    } else if (parser->isSet(QStringLiteral("region"))) {
        lCaptureMode = Spectacle::CaptureMode::RectangularRegion;
    } else if (parser->isSet(QStringLiteral("windowundercursor"))) {
        lCaptureMode = Spectacle::CaptureMode::TransientWithParent;
    } else if (parser->isSet(QStringLiteral("transientonly"))) {
        lCaptureMode = Spectacle::CaptureMode::WindowUnderCursor;
    } else if (mStartMode == SpectacleCore::StartMode::Gui
               && (parser->isSet(QStringLiteral("launchonly")) || Settings::onLaunchAction() == Settings::EnumOnLaunchAction::DoNotTakeScreenshot)
               && !mEditExisting) {
        initGuiNoScreenshot();
        return;
    } else if (Settings::onLaunchAction() == Settings::EnumOnLaunchAction::UseLastUsedCapturemode && !mEditExisting) {
        lCaptureMode = Settings::captureMode();
        if (Settings::onClickChecked()) {
            lDelayMsec = -1;
            takeNewScreenshot(lCaptureMode, lDelayMsec, Settings::includePointer(), Settings::includeDecorations());
        }
    }

    auto lExportManager = ExportManager::instance();
    lExportManager->setCaptureMode(lCaptureMode);

    switch (mStartMode) {
    case StartMode::DBus:
        // if both mCopyImageToClipboard and mSaveToOutput are false, image will only be copied to clipboard
        mCopyImageToClipboard = Settings::clipboardGroup() == Settings::EnumClipboardGroup::PostScreenshotCopyImage;
        mCopyLocationToClipboard = Settings::clipboardGroup() == Settings::EnumClipboardGroup::PostScreenshotCopyLocation;
        mSaveToOutput = Settings::autoSaveImage();

        qApp->setQuitOnLastWindowClosed(false);
        break;

    case StartMode::Background: {
        mCopyImageToClipboard = false;
        mCopyLocationToClipboard = false;
        mSaveToOutput = true;

        if (parser->isSet(QStringLiteral("nonotify"))) {
            mNotify = false;
        }

        if (parser->isSet(QStringLiteral("copy-image"))) {
            mSaveToOutput = false;
            mCopyImageToClipboard = true;
        } else if (parser->isSet(QStringLiteral("copy-path"))) {
            mCopyLocationToClipboard = true;
        }

        if (parser->isSet(QStringLiteral("output"))) {
            mSaveToOutput = true;
            QString lFileName = parser->value(QStringLiteral("output"));
            if (!(lFileName.isEmpty() || lFileName.isNull())) {
                if (QDir::isRelativePath(lFileName)) {
                    lFileName = QDir::current().absoluteFilePath(lFileName);
                }
                setFilename(lFileName);
            }
        }

        if (parser->isSet(QStringLiteral("delay"))) {
            bool lParseOk = false;
            qint64 lDelayValue = parser->value(QStringLiteral("delay")).toLongLong(&lParseOk);
            if (lParseOk) {
                lDelayMsec = lDelayValue;
            }
        }

        if (parser->isSet(QStringLiteral("onclick"))) {
            lDelayMsec = -1;
        }

        if (!mIsGuiInited) {
            static_cast<QGuiApplication *>(qApp->instance())->setQuitOnLastWindowClosed(false);
        }

        auto lIncludePointer = false;
        auto lIncludeDecorations = true;

        if (parser->isSet(QStringLiteral("pointer"))) {
            lIncludePointer = true;
        }

        if (parser->isSet(QStringLiteral("no-decoration"))) {
            lIncludeDecorations = false;
        }

        takeNewScreenshot(lCaptureMode, lDelayMsec, lIncludePointer, lIncludeDecorations);
    } break;

    case StartMode::Gui:
        if (!mIsGuiInited) {
            initGui(lDelayMsec, Settings::includePointer(), Settings::includeDecorations());
        } else {
            using Actions = Settings::EnumPrintKeyActionRunning;
            switch (Settings::printKeyActionRunning()) {
            case Actions::TakeNewScreenshot: {
                // 0 means Immediate, -1 onClick
                int timeout = mPlatform->supportedShutterModes().testFlag(Platform::ShutterMode::Immediate) ? 0 : -1;
                takeNewScreenshot(Settings::captureMode(), timeout, Settings::includePointer(), Settings::includeDecorations());
                break;
            }
            case Actions::FocusWindow:
                if (mMainWindow->isHidden()) {
                    mMainWindow->show();
                }
                if (mMainWindow->isMinimized()) {
                    mMainWindow->setWindowState(mMainWindow->windowState() & ~Qt::WindowMinimized);
                }
                mMainWindow->activateWindow();
                break;
            case Actions::StartNewInstance:
                QProcess newInstance;
                newInstance.setProgram(QCoreApplication::applicationFilePath());
                newInstance.setArguments({QStringLiteral("--new-instance")});
                newInstance.startDetached();
                break;
            }
        }

        break;
    }
}

void SpectacleCore::setUpShortcuts()
{
    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->openAction(), Qt::Key_Print);

    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->fullScreenAction(), Qt::SHIFT | Qt::Key_Print);

    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->activeWindowAction(), Qt::META | Qt::Key_Print);

    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->windowUnderCursorAction(), Qt::META | Qt::CTRL | Qt::Key_Print);

    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->regionAction(), Qt::META | Qt::SHIFT | Qt::Key_Print);

    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->currentScreenAction(), QList<QKeySequence>());

    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->openWithoutScreenshotAction(), QList<QKeySequence>());
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

void SpectacleCore::takeNewScreenshot(Spectacle::CaptureMode theCaptureMode, int theTimeout, bool theIncludePointer, bool theIncludeDecorations)
{
    ExportManager::instance()->setCaptureMode(theCaptureMode);
    auto lGrabMode = toPlatformGrabMode(theCaptureMode);

    if (theTimeout < 0 || !mPlatform->supportedShutterModes().testFlag(Platform::ShutterMode::Immediate)) {
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

void SpectacleCore::screenshotsUpdated(const QVector<QImage> &imgs)
{
    QMap<const QScreen *, QImage> mapScreens;
    QList<QScreen *> screens = QGuiApplication::screens();

    if (imgs.length() != screens.size()) {
        qWarning(SPECTACLE_CORE_LOG()) << "ERROR: images received from KWin do not match, expected:" << imgs.length() << "actual:" << screens.size();
        return;
    }

    // only used by Spectacle::CaptureMode::RectangularRegion
    auto it = imgs.constBegin();
    for (const QScreen *screen : screens) {
        mapScreens.insert(screen, *it);
        ++it;
    }

    mQuickEditor = std::make_unique<QuickEditor>(mapScreens, mWaylandPlasmashell);
    connect(mQuickEditor.get(), &QuickEditor::grabDone, this, &SpectacleCore::screenshotUpdated);
    connect(mQuickEditor.get(), &QuickEditor::grabCancelled, this, &SpectacleCore::screenshotCanceled);
    mQuickEditor->show();
}

void SpectacleCore::screenshotUpdated(const QPixmap &thePixmap)
{
    QPixmap existingPixmap;
    const QPixmap &pixmapUsed = (mEditExisting && !mExistingLoaded) ? existingPixmap : thePixmap;
    if (mEditExisting && !mExistingLoaded) {
        existingPixmap.load(filename());
    }

    auto lExportManager = ExportManager::instance();

    if (lExportManager->captureMode() == Spectacle::CaptureMode::RectangularRegion) {
        if (mQuickEditor) {
            mQuickEditor->hide();
            mQuickEditor.reset(nullptr);
        }
    }

    lExportManager->setPixmap(pixmapUsed);
    lExportManager->updatePixmapTimestamp();

    switch (mStartMode) {
    case StartMode::Background:
    case StartMode::DBus: {
        if (mSaveToOutput || !mCopyImageToClipboard || (Settings::autoSaveImage() && !mSaveToOutput)) {
            mSaveToOutput = Settings::autoSaveImage();
            QUrl lSavePath = (mStartMode == StartMode::Background && mFileNameUrl.isValid() && mFileNameUrl.isLocalFile()) ? mFileNameUrl : QUrl();
            lExportManager->doSave(lSavePath, mNotify);
        }

        if (mCopyImageToClipboard) {
            lExportManager->doCopyToClipboard(mNotify);
        } else if (mCopyLocationToClipboard) {
            lExportManager->doCopyLocationToClipboard(mNotify);
        }

        // if we don't have a Gui already opened, Q_EMIT allDone
        if (!mIsGuiInited) {
            // if we notify, we Q_EMIT allDone only if the user either dismissed the notification or pressed
            // the "Open" button, otherwise the app closes before it can react to it.
            if (!mNotify && mCopyImageToClipboard) {
                // Allow some time for clipboard content to transfer if '--nonotify' is used, see Bug #411263
                // TODO: Find better solution
                QTimer::singleShot(250, this, &SpectacleCore::allDone);
            } else if (!mNotify) {
                Q_EMIT allDone();
            }
        }
    } break;
    case StartMode::Gui:
        if (pixmapUsed.isNull()) {
            mMainWindow->setScreenshotAndShow(pixmapUsed, false);
            mMainWindow->setPlaceholderTextOnLaunch();
            return;
        }
        mMainWindow->setScreenshotAndShow(pixmapUsed, mEditExisting);

        mSaveToOutput = Settings::autoSaveImage();
        mCopyImageToClipboard = Settings::clipboardGroup() == Settings::EnumClipboardGroup::PostScreenshotCopyImage;
        mCopyLocationToClipboard = Settings::clipboardGroup() == Settings::EnumClipboardGroup::PostScreenshotCopyLocation;

        if (mSaveToOutput && mCopyImageToClipboard) {
            lExportManager->doSaveAndCopy();
        } else if (mSaveToOutput) {
            lExportManager->doSave();
        } else if (mCopyImageToClipboard) {
            lExportManager->doCopyToClipboard(false);
        } else if (mCopyLocationToClipboard) {
            lExportManager->doCopyLocationToClipboard(false);
        }
    }

    if (mEditExisting && !mExistingLoaded) {
        Settings::setLastSaveLocation(mFileNameUrl);
        mMainWindow->imageSaved(mFileNameUrl);
        mExistingLoaded = true;
    }
}

void SpectacleCore::screenshotCanceled()
{
    mQuickEditor->hide();
    mQuickEditor.reset(nullptr);
    if (mStartMode == StartMode::Gui) {
        mMainWindow->setScreenshotAndShow(QPixmap(), false);
    } else {
        Q_EMIT allDone();
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
        Q_EMIT allDone();
        return;
    case StartMode::DBus:
        Q_EMIT grabFailed();
        Q_EMIT allDone();
        return;
    case StartMode::Gui:
        mMainWindow->screenshotFailed();
        mMainWindow->setScreenshotAndShow(QPixmap(), false);
    }
}

void SpectacleCore::doNotify(const QUrl &theSavedAt)
{
    KNotification *lNotify = new KNotification(QStringLiteral("newScreenshotSaved"));

    switch (ExportManager::instance()->captureMode()) {
    case Spectacle::CaptureMode::AllScreens:
    case Spectacle::CaptureMode::AllScreensScaled:
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

    // a speaking message is prettier than a URL, special case for copy image/location to clipboard and the default pictures location
    const QString &lSavePath = theSavedAt.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).path();

    if (mCopyImageToClipboard && theSavedAt.fileName().isEmpty()) {
        lNotify->setText(i18n("A screenshot was copied to your clipboard."));
    } else if (mCopyLocationToClipboard && !theSavedAt.fileName().isEmpty()) {
        lNotify->setText(i18n("A screenshot was saved as '%1' to '%2' and the file path of the screenshot has been copied to your clipboard.",
                              theSavedAt.fileName(),
                              lSavePath));
    } else if (lSavePath == QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)) {
        lNotify->setText(i18nc("Placeholder is filename", "A screenshot was saved as '%1' to your Pictures folder.", theSavedAt.fileName()));
    } else if (!theSavedAt.fileName().isEmpty()) {
        lNotify->setText(i18n("A screenshot was saved as '%1' to '%2'.", theSavedAt.fileName(), lSavePath));
    }

    if (!theSavedAt.isEmpty()) {
        lNotify->setUrls({theSavedAt});
        lNotify->setDefaultAction(i18nc("Open the screenshot we just saved", "Open"));
        connect(lNotify, &KNotification::defaultActivated, this, [this, theSavedAt]() {
            auto job = new KIO::OpenUrlJob(theSavedAt);
            job->start();
            QTimer::singleShot(250, this, [this] {
                if (!mIsGuiInited || Settings::quitAfterSaveCopyExport()) {
                    Q_EMIT allDone();
                }
            });
        });
        lNotify->setActions({i18n("Annotate")});
        connect(lNotify, &KNotification::action1Activated, this, [this, theSavedAt]() {
            QProcess newInstance;
            newInstance.setProgram(QCoreApplication::applicationFilePath());
            newInstance.setArguments({QStringLiteral("--new-instance"), QStringLiteral("--edit-existing"), theSavedAt.toLocalFile()});
            newInstance.startDetached();
        });
    }

    connect(lNotify, &QObject::destroyed, this, [this] {
        if (!mIsGuiInited || Settings::quitAfterSaveCopyExport()) {
            Q_EMIT allDone();
        }
    });

    lNotify->sendEvent();
}

void SpectacleCore::doCopyPath(const QUrl &savedAt)
{
    if (Settings::clipboardGroup() == Settings::EnumClipboardGroup::PostScreenshotCopyLocation) {
        qApp->clipboard()->setText(savedAt.toLocalFile());
    }
}

void SpectacleCore::populateCommandLineParser(QCommandLineParser *lCmdLineParser)
{
    lCmdLineParser->addOptions({
        {{QStringLiteral("f"), QStringLiteral("fullscreen")}, i18n("Capture the entire desktop (default)")},
        {{QStringLiteral("m"), QStringLiteral("current")}, i18n("Capture the current monitor")},
        {{QStringLiteral("a"), QStringLiteral("activewindow")}, i18n("Capture the active window")},
        {{QStringLiteral("u"), QStringLiteral("windowundercursor")}, i18n("Capture the window currently under the cursor, including parents of pop-up menus")},
        {{QStringLiteral("t"), QStringLiteral("transientonly")}, i18n("Capture the window currently under the cursor, excluding parents of pop-up menus")},
        {{QStringLiteral("r"), QStringLiteral("region")}, i18n("Capture a rectangular region of the screen")},
        {{QStringLiteral("l"), QStringLiteral("launchonly")}, i18n("Launch Spectacle without taking a screenshot")},
        {{QStringLiteral("g"), QStringLiteral("gui")}, i18n("Start in GUI mode (default)")},
        {{QStringLiteral("b"), QStringLiteral("background")}, i18n("Take a screenshot and exit without showing the GUI")},
        {{QStringLiteral("s"), QStringLiteral("dbus")}, i18n("Start in DBus-Activation mode")},
        {{QStringLiteral("n"), QStringLiteral("nonotify")}, i18n("In background mode, do not pop up a notification when the screenshot is taken")},
        {{QStringLiteral("o"), QStringLiteral("output")}, i18n("In background mode, save image to specified file"), QStringLiteral("fileName")},
        {{QStringLiteral("d"), QStringLiteral("delay")},
         i18n("In background mode, delay before taking the shot (in milliseconds)"),
         QStringLiteral("delayMsec")},
        {{QStringLiteral("c"), QStringLiteral("copy-image")}, i18n("In background mode, copy screenshot image to clipboard, unless -o is also used.")},
        {{QStringLiteral("C"), QStringLiteral("copy-path")}, i18n("In background mode, copy screenshot file path to clipboard")},
        {{QStringLiteral("w"), QStringLiteral("onclick")}, i18n("Wait for a click before taking screenshot. Invalidates delay")},
        {{QStringLiteral("i"), QStringLiteral("new-instance")}, i18n("Starts a new GUI instance of spectacle without registering to DBus")},
        {{QStringLiteral("p"), QStringLiteral("pointer")}, i18n("In background mode, include pointer in the screenshot")},
        {{QStringLiteral("e"), QStringLiteral("no-decoration")}, i18n("In background mode, exclude decorations in the screenshot")},
        {{QStringLiteral("E"), QStringLiteral("edit-existing")}, i18n("Open and edit existing screenshot file"), QStringLiteral("existingFileName")},
    });
}

void SpectacleCore::doStartDragAndDrop()
{
    auto lExportManager = ExportManager::instance();
    if (lExportManager->pixmap().isNull()) {
        return;
    }
    QUrl lTempFile = lExportManager->tempSave();
    if (!lTempFile.isValid()) {
        return;
    }

    auto lMimeData = new QMimeData;
    lMimeData->setUrls(QList<QUrl>{lTempFile});
    lMimeData->setData(QStringLiteral("application/x-kde-suggestedfilename"), QFile::encodeName(lTempFile.fileName()));
    KUrlMimeData::exportUrlsToPortal(lMimeData);

    auto lDragHandler = new QDrag(this);
    lDragHandler->setMimeData(lMimeData);
    lDragHandler->setPixmap(lExportManager->pixmap().scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    lDragHandler->exec(Qt::CopyAction);
}

// Private

Platform::GrabMode SpectacleCore::toPlatformGrabMode(Spectacle::CaptureMode theCaptureMode)
{
    switch (theCaptureMode) {
    case Spectacle::CaptureMode::InvalidChoice:
        return Platform::GrabMode::InvalidChoice;
    case Spectacle::CaptureMode::AllScreens:
        return Platform::GrabMode::AllScreens;
    case Spectacle::CaptureMode::AllScreensScaled:
        return Platform::GrabMode::AllScreensScaled;
    case Spectacle::CaptureMode::RectangularRegion:
        return Platform::GrabMode::PerScreenImageNative;
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

void SpectacleCore::ensureGuiInitiad()
{
    if (!mIsGuiInited) {
        mMainWindow = std::make_unique<KSMainWindow>(mPlatform->supportedGrabModes(), mPlatform->supportedShutterModes());

        connect(mMainWindow.get(), &KSMainWindow::newScreenshotRequest, this, &SpectacleCore::takeNewScreenshot);
        connect(mMainWindow.get(), &KSMainWindow::dragAndDropRequest, this, &SpectacleCore::doStartDragAndDrop);

        mIsGuiInited = true;
    }
}

void SpectacleCore::initGui(int theDelay, bool theIncludePointer, bool theIncludeDecorations)
{
    ensureGuiInitiad();

    takeNewScreenshot(ExportManager::instance()->captureMode(), theDelay, theIncludePointer, theIncludeDecorations);
}

void SpectacleCore::initGuiNoScreenshot()
{
    ensureGuiInitiad();
    screenshotUpdated(QPixmap());
}
