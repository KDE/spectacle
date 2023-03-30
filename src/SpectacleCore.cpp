/*
 *  SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SpectacleCore.h"
#include "CaptureModeModel.h"
#include "CommandLineOptions.h"
#include "Gui/Annotations/AnnotationViewport.h"
#include "Gui/CaptureWindow.h"
#include "Gui/Selection.h"
#include "Gui/SelectionEditor.h"
#include "Gui/SpectacleImageProvider.h"
#include "Gui/SpectacleWindow.h"
#include "ShortcutActions.h"
// generated
#include "Config.h"
#include "settings.h"
#include "spectacle_core_debug.h"

#include <KFormat>
#include <KGlobalAccel>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KMessageBox>
#include <KNotification>
#include <KWindowSystem>
#include <KX11Extras>

#include <QApplication>
#include <QClipboard>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDir>
#include <QDrag>
#include <QKeySequence>
#include <QMimeData>
#include <QProcess>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QScopedPointer>
#include <QScreen>
#include <QTimer>
#include <QtMath>
#include <utility>

SpectacleCore *SpectacleCore::s_self = nullptr;

SpectacleCore::SpectacleCore(QObject *parent)
    : QObject(parent)
{
    s_self = this;
    m_annotationSyncTimer = std::make_unique<QTimer>(new QTimer(this));
    m_annotationSyncTimer->setInterval(400);
    m_annotationSyncTimer->setSingleShot(true);

    m_delayAnimation = std::make_unique<QVariantAnimation>(this);
    m_delayAnimation->setStartValue(0.0);
    m_delayAnimation->setEndValue(1.0);
    m_delayAnimation->setDuration(1);
    m_delayAnimation->setCurrentTime(0);
    auto delayAnimation = m_delayAnimation.get();
    // We need to reset this on start in case a previous instance
    // didn't reset these before it closed or crashed.
    unityLauncherUpdate({
        {QStringLiteral("progress-visible"), false},
        {QStringLiteral("progress"), 0}
    });
    using State = QVariantAnimation::State;
    auto onStateChanged = [this](State newState, State oldState) {
        Q_UNUSED(oldState)
        if (newState == State::Running) {
            unityLauncherUpdate({{QStringLiteral("progress-visible"), true}});
        } else if (newState == State::Stopped) {
            unityLauncherUpdate({{QStringLiteral("progress-visible"), false}});
            m_delayAnimation->setCurrentTime(0);
        }
    };
    auto onValueChanged = [this](const QVariant &value) {
        Q_EMIT captureTimeRemainingChanged();
        Q_EMIT captureProgressChanged();
        unityLauncherUpdate({{QStringLiteral("progress"), value.toReal()}});
        const auto windows = SpectacleWindow::instances();
        if (m_delayAnimation->state() != State::Stopped && !windows.isEmpty()) {
            if (captureTimeRemaining() <= 500 && windows.constFirst()->isVisible()) {
                SpectacleWindow::setVisibilityForAll(QWindow::Hidden);
            }
            SpectacleWindow::setTitleForAll(SpectacleWindow::Timer);
        }
    };
    auto onFinished = [this]() {
        m_platform->doGrab(Platform::ShutterMode::Immediate, m_lastGrabMode,
                           m_lastIncludePointer, m_lastIncludeDecorations);
    };
    QObject::connect(delayAnimation, &QVariantAnimation::stateChanged,
                     this, onStateChanged, Qt::QueuedConnection);
    QObject::connect(delayAnimation, &QVariantAnimation::valueChanged,
                     this, onValueChanged, Qt::QueuedConnection);
    QObject::connect(delayAnimation, &QVariantAnimation::finished,
                     this, onFinished, Qt::QueuedConnection);

    m_platform = loadPlatform();
    m_videoPlatform = loadVideoPlatform();
    auto platform = m_platform.get();
    m_annotationDocument = std::make_unique<AnnotationDocument>(new AnnotationDocument(this));

    // essential connections
    connect(this, &SpectacleCore::errorMessage, this, &SpectacleCore::showErrorMessage);
    connect(this, &SpectacleCore::grabDone, this, [this](const QPixmap &pixmap){
        // only clear images because we're transitioning from rectangle capture to image view.
        m_annotationDocument->clearImages();
        if (m_startMode != StartMode::Gui) {
            SpectacleWindow::setVisibilityForAll(QWindow::Hidden);
        }
        onScreenshotUpdated(pixmap);
    });

    connect(platform, &Platform::newScreenshotTaken, this, [this](const QPixmap &pixmap){
        m_annotationDocument->clear();
        onScreenshotUpdated(pixmap);
        setVideoMode(false);
    });
    connect(platform, &Platform::newScreensScreenshotTaken, this, [this](const QVector<ScreenImage> &screenImages) {
        SelectionEditor::instance()->setScreenImages(screenImages);
        m_annotationDocument->clear();
        for (const auto &img : screenImages) {
            QImage image(img.image);
            if (KWindowSystem::isPlatformWayland()) {
                image.setDevicePixelRatio(qreal(image.width()) / img.screen->geometry().width());
            } else {
                image.setDevicePixelRatio(qApp->devicePixelRatio());
            }
            m_annotationDocument->addImage(image, img.screen->geometry().topLeft());
        }

        auto remember = Settings::rememberLastRectangularRegion();
        if (remember == Settings::Never) {
            SelectionEditor::instance()->selection()->setRect({});
        } else if (remember == Settings::Always) {
            SelectionEditor::instance()->selection()->setRect(Settings::cropRegion());
        }

        initCaptureWindows(CaptureWindow::Image);
        SpectacleWindow::setVisibilityForAll(QWindow::FullScreen);
    });
    connect(platform, &Platform::newScreenshotFailed, this, &SpectacleCore::onScreenshotFailed);

    // set up the export manager
    auto exportManager = ExportManager::instance();
    connect(exportManager, &ExportManager::errorMessage, this, &SpectacleCore::showErrorMessage);
    connect(exportManager, &ExportManager::forceNotify, this, &SpectacleCore::doNotify);
    connect(platform, &Platform::windowTitleChanged, exportManager, &ExportManager::setWindowTitle);
    connect(m_annotationDocument.get(), &AnnotationDocument::repaintNeeded, m_annotationSyncTimer.get(), qOverload<>(&QTimer::start));
    connect(m_annotationSyncTimer.get(), &QTimer::timeout, this, &SpectacleCore::syncExportPixmap);

    connect(exportManager, &ExportManager::imageSaved, this, [](const QUrl &savedAt){
        // This behavior has no relation to the setting in the config UI,
        // but this was behavior was added to solve this feature request:
        // https://bugs.kde.org/show_bug.cgi?id=357423
        if (Settings::clipboardGroup() == Settings::EnumClipboardGroup::PostScreenshotCopyLocation) {
            qApp->clipboard()->setText(savedAt.toLocalFile());
        }
        SpectacleWindow::setTitleForAll(SpectacleWindow::Saved, savedAt.fileName());
        if (ViewerWindow::instance()) {
            ViewerWindow::instance()->showSavedScreenshotMessage(savedAt);
        }
    });
    connect(exportManager, &ExportManager::imageCopied, this, [](){
        if (ViewerWindow::instance()) {
            ViewerWindow::instance()->showCopiedMessage();
        }
    });
    connect(exportManager, &ExportManager::imageLocationCopied, this, [](const QUrl &savedAt){
        SpectacleWindow::setTitleForAll(SpectacleWindow::Saved, savedAt.fileName());
        if (ViewerWindow::instance()) {
            ViewerWindow::instance()->showSavedAndLocationCopiedMessage(savedAt);
        }
    });
    connect(exportManager, &ExportManager::imageSavedAndCopied, this, [](const QUrl &savedAt){
        SpectacleWindow::setTitleForAll(SpectacleWindow::Saved, savedAt.fileName());
        if (ViewerWindow::instance()) {
            ViewerWindow::instance()->showSavedAndCopiedMessage(savedAt);
        }
    });

    // set up shortcuts
    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->openAction(), Qt::Key_Print);
    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->fullScreenAction(), Qt::SHIFT | Qt::Key_Print);
    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->activeWindowAction(), Qt::META | Qt::Key_Print);
    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->windowUnderCursorAction(), Qt::META | Qt::CTRL | Qt::Key_Print);
    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->regionAction(), Qt::META | Qt::SHIFT | Qt::Key_Print);
    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->currentScreenAction(), QList<QKeySequence>());
    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->openWithoutScreenshotAction(), QList<QKeySequence>());

    // set up CaptureMode model
    m_captureModeModel = std::make_unique<CaptureModeModel>(platform->supportedGrabModes(), this);
    m_recordingModeModel = std::make_unique<RecordingModeModel>(m_videoPlatform->supportedRecordingModes(), this);
    auto captureModeModel = m_captureModeModel.get();
    connect(platform, &Platform::supportedGrabModesChanged, captureModeModel, [this](){
        m_captureModeModel->setGrabModes(m_platform->supportedGrabModes());
    });

    connect(qApp, &QApplication::screenRemoved, this, [this](QScreen *screen) {
        // It's dangerous to erase from within a for loop, so we use std::find_if
        auto hasScreen = [screen](const CaptureWindow::UniquePointer &window) {
            return window->screen() == screen;
        };
        auto it = std::find_if(m_captureWindows.begin(), m_captureWindows.end(), hasScreen);
        m_captureWindows.erase(it);
    });

    connect(m_videoPlatform.get(), &VideoPlatform::recordedTimeChanged, this, &SpectacleCore::recordedTimeChanged);
    connect(m_videoPlatform.get(), &VideoPlatform::recordingChanged, this, &SpectacleCore::recordingChanged);
    connect(m_videoPlatform.get(), &VideoPlatform::recordingSaved, this, [this](const QString &path) {
        const QUrl url = QUrl::fromUserInput(path, {}, QUrl::AssumeLocalFile);
        ViewerWindow::instance()->showSavedVideoMessage(url);
        setCurrentVideo(url);
    });
}

SpectacleCore::~SpectacleCore() noexcept
{
    s_self = nullptr;
}

SpectacleCore *SpectacleCore::instance()
{
    return s_self;
}

Platform *SpectacleCore::platform() const
{
    return m_platform.get();
}

CaptureModeModel *SpectacleCore::captureModeModel() const
{
    return m_captureModeModel.get();
}

RecordingModeModel *SpectacleCore::recordingModeModel() const
{
    return m_recordingModeModel.get();
}

AnnotationDocument *SpectacleCore::annotationDocument() const
{
    return m_annotationDocument.get();
}

QUrl SpectacleCore::screenCaptureUrl() const
{
    return m_screenCaptureUrl;
}

void SpectacleCore::setScreenCaptureUrl(const QUrl &url)
{
    if(m_screenCaptureUrl == url) {
        return;
    }
    m_screenCaptureUrl = url;
    Q_EMIT screenCaptureUrlChanged();
}

void SpectacleCore::setScreenCaptureUrl(const QString &filePath)
{
    if (QDir::isRelativePath(filePath)) {
        setScreenCaptureUrl(QUrl::fromUserInput(QDir::current().absoluteFilePath(filePath)));
    } else {
        setScreenCaptureUrl(QUrl::fromUserInput(filePath));
    }
}

int SpectacleCore::captureTimeRemaining() const
{
    int totalDuration = m_delayAnimation->totalDuration();
    int currentTime = m_delayAnimation->currentTime();
    return currentTime > totalDuration || m_delayAnimation->state() == QVariantAnimation::Stopped ?
        0 : totalDuration - currentTime;
}

qreal SpectacleCore::captureProgress() const
{
    // using currentValue() sometimes gives 1.0 when we don't want it.
    return m_delayAnimation->state() == QVariantAnimation::Stopped ?
        0 : m_delayAnimation->currentValue().toReal();
}

void SpectacleCore::activate(const QStringList &arguments, const QString &workingDirectory)
{
    if (!workingDirectory.isEmpty()) {
        QDir::setCurrent(workingDirectory);
    }

    // We can't re-use QCommandLineParser instances, it preserves earlier parsed values
    QCommandLineParser parser;
    parser.addOptions(CommandLineOptions::self()->allOptions);
    // In KF5, KDBusService::activateRequested() will only send a list of arguments if activated
    // via the command line with arguments.
    // Until KF6, we need to check if the arguments list is empty to keep QCommandLineParser from
    // warning us about the arguments being empty.
    // In KF6, KDBusService::activateRequested() will at least send spectacle's executable as an argument
    //  which is the first argument of QCoreApplication::arguments().
    parser.parse(arguments.isEmpty() ? QStringList(QCoreApplication::arguments()[0]) : arguments);

    // Collect parsed command line options
    using Option = CommandLineOptions::Option;
    m_cliOptions.fill(false); // reset all values to false
    int optionsToCheck = parser.optionNames().size();
    for (int i = 0; optionsToCheck > 0 && i < CommandLineOptions::self()->allOptions.size(); ++i) {
        m_cliOptions[i] = parser.isSet(CommandLineOptions::self()->allOptions[i]);
        if (m_cliOptions[i]) {
            --optionsToCheck;
        }
    }

    // No existing screen capture loaded.
    m_existingLoaded = false;

    // Determine start mode
    m_startMode = StartMode::Gui; // Default to Gui
    // Gui is an option that's normally useless since it's the default mode.
    // Make it override the other modes if explicitly set or using the launchonly option.
    if (!m_cliOptions[Option::Gui] && !m_cliOptions[Option::LaunchOnly]) {
        // Background gets precidence over DBus
        if (m_cliOptions[Option::Background]) {
            m_startMode = StartMode::Background;
        } else if (m_cliOptions[Option::DBus]) {
            m_startMode = StartMode::DBus;
        }
    }

    if (parser.optionNames().size() > 0 || m_startMode != StartMode::Gui) {
        // Delete windows if we have CLI options or not in GUI mode.
        // We don't want to delete them otherwise because that will mess with the
        // settings for PrintScreen key behavior.
        deleteWindows();
    }

    /* The logic for setting options for each start mode:
     *
     * - Gui/DBus: Prioritise command line options and default to saved settings.
     * - Background: Prioritise command line options and use defaults based on
     *   how command line options are meant to be used.
     *
     * Never start with a delay by default. It is annoying and confuses users
     * when nothing happens immediately after starting spectacle.
     */

    // In the GUI/CLI, the TransientWithParent mode is represented by the
    // "Window Under Cursor" option and the real WindowUnderCursor mode is
    // represented by the popup-only/transientOnly setting, which is meant to
    // override TransientWithParent. Needless to say, This is rather convoluted.
    // TODO: Improve the API for transientOnly or make it obsolete.
    bool transientOnly;
    bool onClick;
    bool includeDecorations;
    bool includePointer;
    if (m_startMode == StartMode::Background) {
        transientOnly = m_cliOptions[Option::TransientOnly];
        onClick = m_cliOptions[Option::OnClick];
        includeDecorations = !m_cliOptions[Option::NoDecoration];
        includePointer = m_cliOptions[Option::Pointer];
        setSaveCopyImageCopyPath(m_cliOptions[Option::Output],
                                 m_cliOptions[Option::CopyImage],
                                 m_cliOptions[Option::CopyPath]);
    } else {
        transientOnly = Settings::transientOnly() || m_cliOptions[Option::TransientOnly];
        onClick = Settings::captureOnClick() || m_cliOptions[Option::OnClick];
        includeDecorations = Settings::includeDecorations()
                            && !m_cliOptions[Option::NoDecoration];
        includePointer = Settings::includePointer() || m_cliOptions[Option::Pointer];
        setSaveCopyImageCopyPath(m_cliOptions[Option::Output] || Settings::autoSaveImage(),
                                 m_cliOptions[Option::CopyImage] || Settings::clipboardGroup() == Settings::PostScreenshotCopyImage,
                                 m_cliOptions[Option::CopyPath] || Settings::clipboardGroup() == Settings::PostScreenshotCopyLocation);
    }

    int delayMsec = 0; // default to 0 if cli value parse fails
    if (onClick) {
        delayMsec = -1;
    } else if (m_cliOptions[Option::Delay]) {
        bool parseOk = false;
        int value = parser.value(CommandLineOptions::self()->delay).toInt(&parseOk);
        if (parseOk) {
            delayMsec = value;
        }
    }

    m_editExisting = m_cliOptions[Option::EditExisting];
    if (m_editExisting) {
        QString existingFileName = parser.value(CommandLineOptions::self()->editExisting);
        if (!(existingFileName.isEmpty() || existingFileName.isNull())) {
            setScreenCaptureUrl(existingFileName);
            m_saveToOutput = true;
        }
    }

    // Determine grab mode
    using CaptureMode = CaptureModeModel::CaptureMode;
    using GrabMode = Platform::GrabMode;
    GrabMode grabMode = GrabMode::AllScreens; // Default to all screens
    if (m_cliOptions[Option::Fullscreen]) {
        grabMode = GrabMode::AllScreens;
    } else if (m_cliOptions[Option::Current]) {
        grabMode = GrabMode::CurrentScreen;
    } else if (m_cliOptions[Option::ActiveWindow]) {
        grabMode = GrabMode::ActiveWindow;
    } else if (m_cliOptions[Option::Region]) {
        grabMode = GrabMode::PerScreenImageNative;
    } else if (m_cliOptions[Option::WindowUnderCursor]) {
        grabMode = GrabMode::WindowUnderCursor;
    } else if (Settings::launchAction() == Settings::UseLastUsedCapturemode) {
        grabMode = toGrabMode(CaptureMode(Settings::captureMode()), transientOnly);
    }
    ExportManager::instance()->setWindowTitle({});

    // reset last region if it should not be remembered across restarts
    if (!(Settings::rememberLastRectangularRegion() == Settings::EnumRememberLastRectangularRegion::Always)) {
        // QRect defaults to {0,0,-1,-1} while QRectF defaults to {0,0,0,0}.
        Settings::setCropRegion({0, 0, 0, 0});
    }

    switch (m_startMode) {
    case StartMode::DBus:
        m_notify = !m_cliOptions[Option::NoNotify];
        break;

    case StartMode::Background: {
        m_notify = !m_cliOptions[Option::NoNotify];

        if (m_saveToOutput) {
            QString lFileName = parser.value(CommandLineOptions::self()->output);
            if (!(lFileName.isEmpty() || lFileName.isNull())) {
                setScreenCaptureUrl(lFileName);
            }
        }

        takeNewScreenshot(grabMode, delayMsec, includePointer, includeDecorations);
    } break;

    case StartMode::Gui:
        m_notify = Settings::quitAfterSaveCopyExport() && !m_cliOptions[Option::NoNotify];
        if (isGuiNull()) {
            if ((m_cliOptions[Option::LaunchOnly]
                    || Settings::launchAction() == Settings::DoNotTakeScreenshot)
                && !m_editExisting
            ) {
                initViewerWindow(ViewerWindow::Dialog);
                ViewerWindow::instance()->setVisible(true);
            } else {
                takeNewScreenshot(grabMode, delayMsec, includePointer, includeDecorations);
            }
        } else {
            using Actions = Settings::EnumPrintKeyActionRunning;
            switch (Settings::printKeyActionRunning()) {
            case Actions::TakeNewScreenshot: {
                // takeNewScreenshot switches to on click if immediate is not supported.
                takeNewScreenshot(grabMode, 0, includePointer, includeDecorations);
                break;
            }
            case Actions::FocusWindow: {
                bool isCaptureWindow = !CaptureWindow::instances().isEmpty();
                SpectacleWindow *window = nullptr;
                if (isCaptureWindow) {
                    window = CaptureWindow::instances().front();
                } else {
                    window = ViewerWindow::instance();
                }
                if (isCaptureWindow) {
                    SpectacleWindow::setVisibilityForAll(QWindow::FullScreen);
                } else {
                    // Unminimize the window.
                    window->unminimize();
                }
                window->requestActivate();
                break;
            }
            case Actions::StartNewInstance: {
                QProcess newInstance;
                newInstance.setProgram(QCoreApplication::applicationFilePath());
                newInstance.setArguments({
                    CommandLineOptions::toArgument(CommandLineOptions::self()->newInstance)
                });
                newInstance.startDetached();
                break;
            }
            }
        }

        break;
    }
}

void SpectacleCore::takeNewScreenshot(Platform::GrabMode grabMode, int timeout, bool includePointer, bool includeDecorations)
{
    m_delayAnimation->stop();

    m_lastGrabMode = grabMode;
    m_lastIncludePointer = includePointer;
    m_lastIncludeDecorations = includeDecorations;

    if ((timeout < 0 || !m_platform->supportedShutterModes().testFlag(Platform::Immediate))
        && m_platform->supportedShutterModes().testFlag(Platform::OnClick)
    ) {
        SpectacleWindow::setVisibilityForAll(QWindow::Hidden);
        m_platform->doGrab(Platform::ShutterMode::OnClick, m_lastGrabMode, m_lastIncludePointer, m_lastIncludeDecorations);
        return;
    }

    const bool noDelay = timeout == 0;

    // when compositing is enabled, we need to give it enough time for the window
    // to disappear and all the effects are complete before we take the shot. there's
    // no way of knowing how long the disappearing effects take, but as per default
    // settings (and unless the user has set an extremely slow effect), 200
    // milliseconds is a good amount of wait time.
    timeout = qMax(timeout, KX11Extras::compositingActive() ? 200 : 50);

    if (noDelay) {
        SpectacleWindow::setVisibilityForAll(QWindow::Hidden);
        QTimer::singleShot(timeout, this, [this]() {
            m_platform->doGrab(Platform::ShutterMode::Immediate, m_lastGrabMode, m_lastIncludePointer, m_lastIncludeDecorations);
        });
        return;
    }

    m_delayAnimation->setDuration(timeout);
    m_delayAnimation->start();

    SpectacleWindow::setVisibilityForAll(QWindow::Minimized);
}

void SpectacleCore::takeNewScreenshot(int captureMode, int timeout, bool includePointer, bool includeDecorations)
{
    using CaptureMode = CaptureModeModel::CaptureMode;
    takeNewScreenshot(toGrabMode(CaptureMode(captureMode), Settings::transientOnly()),
                      timeout, includePointer, includeDecorations);
}

void SpectacleCore::cancelScreenshot()
{
    if (m_startMode != StartMode::Gui) {
        Q_EMIT allDone();
        return;
    }

    int currentTime = m_delayAnimation->currentTime();
    m_delayAnimation->stop();
    if (currentTime > 0) {
        SpectacleWindow::setTitleForAll(SpectacleWindow::Previous);
    }
}

void SpectacleCore::showErrorMessage(const QString &theErrString)
{
    qCDebug(SPECTACLE_CORE_LOG) << "ERROR: " << theErrString;

    if (m_startMode == StartMode::Gui) {
        KMessageBox::error(nullptr, theErrString);
    }
}

void SpectacleCore::onScreenshotUpdated(const QPixmap &thePixmap)
{
    using Option = CommandLineOptions::Option;
    QPixmap existingPixmap;
    const QPixmap &pixmapUsed = (m_editExisting && !m_existingLoaded) ? existingPixmap : thePixmap;
    if (m_editExisting && !m_existingLoaded) {
        existingPixmap.load(m_screenCaptureUrl.toLocalFile());
    }

    auto exportManager = ExportManager::instance();
    exportManager->setPixmap(pixmapUsed);
    m_annotationDocument->addImage(pixmapUsed.toImage(), QPointF(0, 0));
    exportManager->updatePixmapTimestamp();

    switch (m_startMode) {
    case StartMode::Background:
    case StartMode::DBus: {
        syncExportPixmap();
        if (m_saveToOutput) {
            QUrl lSavePath = (m_startMode == StartMode::Background && m_screenCaptureUrl.isValid() && m_screenCaptureUrl.isLocalFile()) ? m_screenCaptureUrl : QUrl();
            exportManager->doSave(lSavePath, m_notify);
        }
        if (m_copyImageToClipboard) {
            exportManager->doCopyToClipboard(m_notify);
        } else if (m_copyLocationToClipboard) {
            exportManager->doCopyLocationToClipboard(m_notify);
        }

        // if we don't have a Gui already opened, Q_EMIT allDone
        if (isGuiNull()) {
            // if we notify, we Q_EMIT allDone only if the user either dismissed the notification or pressed
            // the "Open" button, otherwise the app closes before it can react to it.
            if (!m_notify && m_copyImageToClipboard) {
                // Allow some time for clipboard content to transfer if '--nonotify' is used, see Bug #411263
                // TODO: Find better solution
                QTimer::singleShot(250, this, &SpectacleCore::allDone);
            } else if (!m_notify) {
                Q_EMIT allDone();
            }
        }
    } break;
    case StartMode::Gui:
        // These can change in GUI mode, so set them again
        m_notify = Settings::quitAfterSaveCopyExport()
                && !m_cliOptions[CommandLineOptions::NoNotify];
        setSaveCopyImageCopyPath(m_cliOptions[Option::Output] || Settings::autoSaveImage(),
                                 m_cliOptions[Option::CopyImage] || Settings::clipboardGroup() == Settings::PostScreenshotCopyImage,
                                 m_cliOptions[Option::CopyPath] || Settings::clipboardGroup() == Settings::PostScreenshotCopyLocation);

        if (pixmapUsed.isNull()) {
            initViewerWindow(ViewerWindow::Dialog);
            ViewerWindow::instance()->setVisible(true);
            return;
        }
        if (!m_editExisting) {
            setScreenCaptureUrl(QUrl(QStringLiteral("image://spectacle/%1").arg(pixmapUsed.cacheKey())));
        }
        initViewerWindow(ViewerWindow::Image);
        if (m_editExisting) {
            ViewerWindow::instance()->setAnnotating(true);
        }
        ViewerWindow::instance()->setVisible(true);
        auto titlePreset = !pixmapUsed.isNull() ? SpectacleWindow::Unsaved : SpectacleWindow::Saved;
        SpectacleWindow::setTitleForAll(titlePreset);

        if (m_saveToOutput && m_copyImageToClipboard) {
            syncExportPixmap();
            exportManager->doSaveAndCopy();
        } else if (m_saveToOutput) {
            exportManager->doSave();
        } else if (m_copyImageToClipboard) {
            syncExportPixmap();
            exportManager->doCopyToClipboard(false);
        }
        // This is a separate block since we don't do anything special for
        // saving and copying the location, unlike saving and copying the image.
        if (m_copyLocationToClipboard) {
            exportManager->doCopyLocationToClipboard(false);
        }
    }

    if (m_editExisting && !m_existingLoaded) {
        Settings::setLastSaveLocation(m_screenCaptureUrl);
        m_existingLoaded = true;
    }
}

void SpectacleCore::onScreenshotFailed()
{
    switch (m_startMode) {
    case StartMode::Background:
        showErrorMessage(i18n("Screenshot capture canceled or failed"));
        Q_EMIT allDone();
        return;
    case StartMode::DBus:
        Q_EMIT grabFailed();
        Q_EMIT allDone();
        return;
    case StartMode::Gui:
        if (!ViewerWindow::instance()) {
            initViewerWindow(ViewerWindow::Dialog);
        }
        ViewerWindow::instance()->showScreenshotFailedMessage();
        return;
    }
}

static QVector<KNotification *> notifications;

void SpectacleCore::doNotify(const QUrl &theSavedAt)
{
    // ensure program stays alive until the notification finishes.
    if (!m_eventLoopLocker) {
        m_eventLoopLocker = std::make_unique<QEventLoopLocker>();
    }

    auto notification = new KNotification(QStringLiteral("newScreenshotSaved"),
                                          KNotification::CloseOnTimeout, this);
    notifications.append(notification);

    int index = captureModeModel()->indexOfCaptureMode(toCaptureMode(m_lastGrabMode));
    auto captureModeLabel = captureModeModel()->data(captureModeModel()->index(index),
                                                     Qt::DisplayRole);
    notification->setTitle(captureModeLabel.toString());

    // a speaking message is prettier than a URL, special case for copy image/location to clipboard and the default pictures location
    const QString &lSavePath = theSavedAt.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).path();

    if (m_copyImageToClipboard && theSavedAt.fileName().isEmpty()) {
        notification->setText(i18n("A screenshot was saved to your clipboard."));
    } else if (m_copyLocationToClipboard && !theSavedAt.fileName().isEmpty()) {
        notification->setText(i18n("A screenshot was saved as '%1' to '%2' and the file path of the screenshot has been saved to your clipboard.",
                              theSavedAt.fileName(),
                              lSavePath));
    } else if (lSavePath == QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)) {
        notification->setText(i18nc("Placeholder is filename", "A screenshot was saved as '%1' to your Pictures folder.", theSavedAt.fileName()));
    } else if (!theSavedAt.fileName().isEmpty()) {
        notification->setText(i18n("A screenshot was saved as '%1' to '%2'.", theSavedAt.fileName(), lSavePath));
    }

    if (!theSavedAt.isEmpty()) {
        notification->setUrls({theSavedAt});
        notification->setDefaultAction(i18nc("Open the screenshot we just saved", "Open"));
        connect(notification, &KNotification::defaultActivated, this, [theSavedAt]() {
            auto job = new KIO::OpenUrlJob(theSavedAt);
            job->start();
        });
        notification->setActions({i18n("Annotate")});
        connect(notification, &KNotification::action1Activated, this, [theSavedAt]() {
            QProcess newInstance;
            newInstance.setProgram(QCoreApplication::applicationFilePath());
            newInstance.setArguments({
                CommandLineOptions::toArgument(CommandLineOptions::self()->newInstance),
                CommandLineOptions::toArgument(CommandLineOptions::self()->editExisting),
                theSavedAt.toLocalFile()
            });
            newInstance.startDetached();
        });
    }

    connect(notification, &QObject::destroyed, this, [this](QObject *notification) {
        notifications.removeOne(static_cast<KNotification *>(notification));
        // When there are no more notifications running, we can remove the loop locker.
        if (notifications.empty()) {
            QTimer::singleShot(250, this, [this] {
                m_eventLoopLocker.reset();
            });
        }
    });

    notification->sendEvent();
}

// In background and dbus mode, ensure that either save or copy image is enabled.
void SpectacleCore::setSaveCopyImageCopyPath(bool save, bool copyImage, bool copyPath)
{
    if (m_startMode == StartMode::Gui) {
        m_saveToOutput = save;
        m_copyImageToClipboard = copyImage;
        m_copyLocationToClipboard = save && copyPath;
    } else {
        m_saveToOutput = save || !copyImage;
        m_copyImageToClipboard = !m_saveToOutput || copyImage;
        m_copyLocationToClipboard = save && copyPath;
    }
}

Platform::GrabMode SpectacleCore::toGrabMode(CaptureModeModel::CaptureMode captureMode, bool transientOnly) const
{
    using GrabMode = Platform::GrabMode;
    using CaptureMode = CaptureModeModel::CaptureMode;
    const auto &supportedGrabModes = m_platform->supportedGrabModes();
    if (captureMode == CaptureMode::CurrentScreen
        && supportedGrabModes.testFlag(Platform::CurrentScreen)) {
        return GrabMode::CurrentScreen;
    } else if (captureMode == CaptureMode::ActiveWindow
        && supportedGrabModes.testFlag(Platform::ActiveWindow)) {
        return GrabMode::ActiveWindow;
    } else if (captureMode == CaptureMode::WindowUnderCursor
        && supportedGrabModes.testFlag(Platform::WindowUnderCursor)) {
        // TODO: Improve API for transientOnly or make it obsolete.
        if (transientOnly || !supportedGrabModes.testFlag(Platform::TransientWithParent)) {
            return GrabMode::WindowUnderCursor;
        } else {
            return GrabMode::TransientWithParent;
        }
    } else if (captureMode == CaptureMode::RectangularRegion
        && supportedGrabModes.testFlag(Platform::PerScreenImageNative)) {
        return GrabMode::PerScreenImageNative;
    } else if (captureMode == CaptureMode::AllScreensScaled
        && supportedGrabModes.testFlag(Platform::AllScreensScaled)) {
        return GrabMode::AllScreensScaled;
    } else if (supportedGrabModes.testFlag(Platform::AllScreens)) { // default if supported
        return GrabMode::AllScreens;
    } else {
        return GrabMode::NoGrabModes;
    }
}

CaptureModeModel::CaptureMode SpectacleCore::toCaptureMode(Platform::GrabMode grabMode) const
{
    using GrabMode = Platform::GrabMode;
    using CaptureMode = CaptureModeModel::CaptureMode;
    if (grabMode == GrabMode::CurrentScreen) {
        return CaptureMode::CurrentScreen;
    } else if (grabMode == GrabMode::ActiveWindow) {
        return CaptureMode::ActiveWindow;
    } else if (grabMode == GrabMode::WindowUnderCursor) {
        return CaptureMode::WindowUnderCursor;
    } else if (grabMode == GrabMode::PerScreenImageNative) {
        return CaptureMode::RectangularRegion;
    } else if (grabMode == GrabMode::AllScreensScaled) {
        return CaptureMode::AllScreensScaled;
    } else {
        return CaptureMode::AllScreens;
    }
}

bool SpectacleCore::isGuiNull() const
{
    return SpectacleWindow::instances().isEmpty();
}

void SpectacleCore::initGuiNoScreenshot()
{
    initViewerWindow(ViewerWindow::Dialog);
    ViewerWindow::instance()->setVisible(true);
}

void SpectacleCore::syncExportPixmap()
{
    qreal maxDpr = 0.0;
    for (auto &img : m_annotationDocument->baseImages()) {
        maxDpr = qMax(maxDpr, img.devicePixelRatio());
    }
    QRectF imageRect(QPointF(0, 0), m_annotationDocument->canvasSize() * maxDpr);
    const auto &image = m_annotationDocument->renderToImage(imageRect, maxDpr);
    ExportManager::instance()->setPixmap(QPixmap::fromImage(image));
}

QQmlEngine *SpectacleCore::getQmlEngine()
{
    if (m_engine == nullptr) {
        m_engine = std::make_unique<QQmlEngine>(this);
        m_engine->addImageProvider(QStringLiteral("spectacle"),
                                   new SpectacleImageProvider(QQmlImageProviderBase::Pixmap));
        m_engine->rootContext()->setContextObject(new KLocalizedContext(m_engine.get()));

        qmlRegisterSingletonInstance(QML_URI_PRIVATE, 1, 0, "SpectacleCore", this);
        qmlRegisterSingletonInstance(QML_URI_PRIVATE, 1, 0, "Platform", m_platform.get());
        qmlRegisterSingletonInstance(QML_URI_PRIVATE, 1, 0, "Settings", Settings::self());
        qmlRegisterSingletonInstance(QML_URI_PRIVATE, 1, 0, "CaptureModeModel", m_captureModeModel.get());
        qmlRegisterSingletonInstance(QML_URI_PRIVATE, 1, 0, "SelectionEditor", SelectionEditor::instance());
        qmlRegisterSingletonInstance(QML_URI_PRIVATE, 1, 0, "Selection", SelectionEditor::instance()->selection());

        qmlRegisterSingletonInstance(QML_URI_PRIVATE, 1, 0, "AnnotationDocument", m_annotationDocument.get());
        qmlRegisterUncreatableType<AnnotationTool>(QML_URI_PRIVATE, 1, 0, "AnnotationTool",
                                                   QStringLiteral("Use AnnotationDocument.tool"));
        qmlRegisterUncreatableType<SelectedActionWrapper>(QML_URI_PRIVATE, 1, 0, "SelectedAction",
                                                          QStringLiteral("Use AnnotationDocument.selectedAction"));
        qmlRegisterType<AnnotationViewport>(QML_URI_PRIVATE, 1, 0, "AnnotationViewport");
    }
    return m_engine.get();
}

void SpectacleCore::initCaptureWindows(CaptureWindow::Mode mode)
{
    deleteWindows();

    // Allow the window to be transparent. Used for video recording UI.
    // It has to be set before creating the window.
    QQuickWindow::setDefaultAlphaBuffer(true);

    auto engine = getQmlEngine();
    const auto screens = qApp->screens();
    for (auto *screen : screens) {
        m_captureWindows.emplace_back(CaptureWindow::makeUnique(mode, screen, engine));
    }
}

void SpectacleCore::initViewerWindow(ViewerWindow::Mode mode)
{
    // always switch to gui mode when a viewer window is used.
    m_startMode = SpectacleCore::StartMode::Gui;
    deleteWindows();

    // Transparency isn't needed for this window.
    QQuickWindow::setDefaultAlphaBuffer(false);

    m_viewerWindow = ViewerWindow::makeUnique(mode, getQmlEngine());
}

void SpectacleCore::deleteWindows()
{
    m_viewerWindow.reset();
    m_captureWindows.clear();
}

void SpectacleCore::unityLauncherUpdate(const QVariantMap &properties) const
{
    QDBusMessage message = QDBusMessage::createSignal(QStringLiteral("/org/kde/Spectacle"),
                                                      QStringLiteral("com.canonical.Unity.LauncherEntry"),
                                                      QStringLiteral("Update"));
    message.setArguments({QApplication::desktopFileName(), properties});
    QDBusConnection::sessionBus().send(message);
}

void SpectacleCore::startRecordingScreen(QScreen *screen, bool withPointer)
{
    Q_ASSERT(!m_videoPlatform->isRecording());
    const QString output = ExportManager::instance()->suggestedVideoFilename(m_videoPlatform->extension());
    m_videoPlatform->startRecording(output, VideoPlatform::Screen, screen, withPointer);
    setVideoMode(true);
}

void SpectacleCore::startRecordingRegion(const QRect &region, bool withPointer)
{
    Q_ASSERT(!m_videoPlatform->isRecording());
    const QString output = ExportManager::instance()->suggestedVideoFilename(m_videoPlatform->extension());
    m_videoPlatform->startRecording(output, VideoPlatform::Region, region, withPointer);
    setVideoMode(true);
}

void SpectacleCore::startRecordingWindow(const QString &uuid, bool withPointer)
{
    Q_ASSERT(!m_videoPlatform->isRecording());
    const QString output = ExportManager::instance()->suggestedVideoFilename(m_videoPlatform->extension());
    m_videoPlatform->startRecording(output, VideoPlatform::Window, uuid, withPointer);
    setVideoMode(true);
}

void SpectacleCore::finishRecording()
{
    Q_ASSERT(m_videoPlatform->isRecording());
    m_videoPlatform->finishRecording();
}

bool SpectacleCore::isRecording() const
{
    return m_videoPlatform->isRecording();
}

bool SpectacleCore::recordingSupported() const
{
    return m_videoPlatform->supportedRecordingModes() != 0;
}

bool SpectacleCore::videoMode() const
{
    return m_videoMode;
}

void SpectacleCore::setVideoMode(bool videoMode)
{
    if (videoMode == m_videoMode) {
        return;
    }
    m_videoMode = videoMode;
    Q_EMIT videoModeChanged(videoMode);
}

QUrl SpectacleCore::currentVideo() const
{
    return m_currentVideo;
}

void SpectacleCore::setCurrentVideo(const QUrl &currentVideo)
{
    if (currentVideo == m_currentVideo) {
        return;
    }
    m_currentVideo = currentVideo;
    Q_EMIT currentVideoChanged(currentVideo);
}

QString SpectacleCore::recordedTime() const
{
    return timeFromMilliseconds(m_videoPlatform->recordedTime());
}

QString SpectacleCore::timeFromMilliseconds(qint64 milliseconds) const
{
    KFormat::DurationFormatOptions options = KFormat::DefaultDuration;
    if (milliseconds < 1000.0 * 60.0 * 60.0) {
        options |= KFormat::FoldHours;
    }
    return KFormat().formatDuration(milliseconds, options);
}
