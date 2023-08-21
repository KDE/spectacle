/*
 *  SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SpectacleCore.h"
#include "CanvasImage.h"
#include "CaptureModeModel.h"
#include "CommandLineOptions.h"
#include "ExportManager.h"
#include "Geometry.h"
#include "Gui/Annotations/AnnotationViewport.h"
#include "Gui/CaptureWindow.h"
#include "Gui/Selection.h"
#include "Gui/SelectionEditor.h"
#include "Gui/SpectacleWindow.h"
#include "ShortcutActions.h"
#include "PlasmaVersion.h"
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
#include <qobjectdefs.h>
#include <utility>

SpectacleCore *SpectacleCore::s_self = nullptr;

SpectacleCore::SpectacleCore(QObject *parent)
    : QObject(parent)
{
    s_self = this;
    // Timer to prevent lots of extra rendering to images
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
    m_videoPlatform->setExtension(Settings::videoFormat());
    auto platform = m_platform.get();
    m_annotationDocument = std::make_unique<AnnotationDocument>(new AnnotationDocument(this));

    // essential connections
    connect(this, &SpectacleCore::errorMessage, this, &SpectacleCore::showErrorMessage);
    connect(this, &SpectacleCore::grabDone, this, [this](const QImage &image,
                                                         const ExportManager::Actions &actions){
        deleteWindows();
        onScreenshotUpdated(image);
        syncExportImage();
        const auto &exportActions = actions & ExportManager::AnyAction ? actions : autoExportActions();
        ExportManager::instance()->exportImage(exportActions, outputUrl());
    });

    connect(platform, &Platform::newScreenshotTaken, this, [this](const QImage &image){
        m_annotationDocument->clearAnnotations();
        onScreenshotUpdated(image);
        syncExportImage();
        ExportManager::instance()->exportImage(autoExportActions(), outputUrl());
        setVideoMode(false);
    });
    connect(platform, &Platform::newScreensScreenshotTaken, this, [this](const QVector<CanvasImage> &screenImages) {
        auto selectionEditor = SelectionEditor::instance();
        auto selection = selectionEditor->selection();
        selectionEditor->setScreenImages(screenImages);
        m_annotationDocument->clear();
        m_annotationDocument->setCanvasImages(screenImages);

        auto remember = Settings::rememberLastRectangularRegion();
        if (remember == Settings::Never) {
            selection->setRect({});
        } else if (remember == Settings::Always) {
            auto cropRegion = Settings::cropRegion();
            if (cropRegion.width() < 0 || cropRegion.height() < 0) {
                cropRegion = {0, 0, 0, 0};
            }
            selection->setRect(cropRegion);
        }

        initCaptureWindows(CaptureWindow::Image);
        SpectacleWindow::setTitleForAll(SpectacleWindow::Unsaved);
        SpectacleWindow::setVisibilityForAll(QWindow::FullScreen);
    });
    connect(platform, &Platform::newScreenshotFailed, this, &SpectacleCore::onScreenshotFailed);

    // set up the export manager
    auto exportManager = ExportManager::instance();
    auto onImageExported = [this](const ExportManager::Actions &actions, const QUrl &url) {
        if (actions & ExportManager::UserAction && Settings::quitAfterSaveCopyExport()) {
            deleteWindows();
        }

        if (isGuiNull()) {
            if (m_cliOptions[CommandLineOptions::NoNotify]) {
                // if we notify, we Q_EMIT allDone only if the user either dismissed the notification or pressed
                // the "Open" button, otherwise the app closes before it can react to it.
                if (actions & ExportManager::CopyImage) {
                    // Allow some time for clipboard content to transfer if '--nonotify' is used, see Bug #411263
                    // TODO: Find better solution
                    QTimer::singleShot(250, this, &SpectacleCore::allDone);
                } else {
                    Q_EMIT allDone();
                }
            } else {
                doNotify(actions, url);
            }
            return;
        }

        auto viewerWindow = ViewerWindow::instance();
        if (!viewerWindow) {
            return;
        }

        if (actions & ExportManager::AnySave) {
            SpectacleWindow::setTitleForAll(SpectacleWindow::Saved, url.fileName());
            if (actions & ExportManager::CopyImage) {
                viewerWindow->showSavedAndCopiedMessage(url);
            } else if (actions & ExportManager::CopyPath) {
                viewerWindow->showSavedAndLocationCopiedMessage(url);
            } else {
                viewerWindow->showSavedScreenshotMessage(url);
            }
        } else if (actions & ExportManager::CopyImage) {
            viewerWindow->showCopiedMessage();
        }
    };
    connect(exportManager, &ExportManager::imageExported, this, onImageExported);
    connect(exportManager, &ExportManager::errorMessage, this, &SpectacleCore::showErrorMessage);

    connect(platform, &Platform::windowTitleChanged, exportManager, &ExportManager::setWindowTitle);
    connect(m_annotationDocument.get(), &AnnotationDocument::repaintNeeded, m_annotationSyncTimer.get(), qOverload<>(&QTimer::start));
    connect(m_annotationSyncTimer.get(), &QTimer::timeout, this, [this] {
        ExportManager::instance()->setImage(m_annotationDocument->renderToImage());
    }, Qt::QueuedConnection); // QueuedConnection to help prevent making the visible render lag.

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
        if (it != m_captureWindows.end()) {
            m_captureWindows.erase(it);
        }
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

QUrl SpectacleCore::outputUrl() const
{
    return m_outputUrl.isEmpty() ? m_editExistingUrl : m_outputUrl;
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

    // Determine start mode
    m_startMode = StartMode::Gui; // Default to Gui
    // Gui is an option that's normally useless since it's the default mode.
    // Make it override the other modes if explicitly set, using the launchonly option,
    // or editing an existing image. Editing an existing image requires a viewer window.
    if (!m_cliOptions[Option::Gui]
        && !m_cliOptions[Option::LaunchOnly]
        && !m_cliOptions[Option::EditExisting]) {
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

    // reset last region if it should not be remembered across restarts
    if (!(Settings::rememberLastRectangularRegion() == Settings::EnumRememberLastRectangularRegion::Always)) {
        // QRect defaults to {0,0,-1,-1} while QRectF defaults to {0,0,0,0}.
        Settings::setCropRegion({0, 0, 0, 0});
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
    } else {
        transientOnly = Settings::transientOnly() || m_cliOptions[Option::TransientOnly];
        onClick = Settings::captureOnClick() || m_cliOptions[Option::OnClick];
        includeDecorations = Settings::includeDecorations()
                            && !m_cliOptions[Option::NoDecoration];
        includePointer = Settings::includePointer() || m_cliOptions[Option::Pointer];
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

    if (m_cliOptions[Option::EditExisting]) {
        auto input = parser.value(CommandLineOptions::self()->editExisting);
        m_editExistingUrl = QUrl::fromUserInput(input, QDir::currentPath(), QUrl::AssumeLocalFile);
        // QFileInfo::exists() only works with local files.
        auto existingLocalFile = m_editExistingUrl.toLocalFile();
        if (QFileInfo::exists(existingLocalFile)) {
            m_annotationDocument->clearAnnotations();
            // If editing an existing image, open the annotation editor.
            // This QImage constructor only works with local files or Qt resource file names.
            onScreenshotUpdated(QImage(existingLocalFile));
            SpectacleWindow::setTitleForAll(SpectacleWindow::Saved, m_editExistingUrl.fileName());
            return;
        } else {
            m_cliOptions[Option::EditExisting] = false;
            m_editExistingUrl.clear();
        }
    } else {
        m_editExistingUrl.clear();
    }

    if (m_cliOptions[Option::Output]) {
        m_outputUrl = QUrl::fromUserInput(parser.value(CommandLineOptions::self()->output),
                                          QDir::currentPath(), QUrl::AssumeLocalFile);
        if (!m_outputUrl.isValid()) {
            m_cliOptions[Option::Output] = false;
            m_outputUrl.clear();
        }
    } else {
        m_outputUrl.clear();
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


    switch (m_startMode) {
    case StartMode::DBus:
        break;
    case StartMode::Background:
        takeNewScreenshot(grabMode, delayMsec, includePointer, includeDecorations);
        break;
    case StartMode::Gui:
        if (isGuiNull()) {
            if ((m_cliOptions[Option::LaunchOnly]
                || Settings::launchAction() == Settings::DoNotTakeScreenshot)
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
    if (m_cliOptions[CommandLineOptions::EditExisting]) {
        // Clear when a new screenshot is taken to avoid overwriting
        // the existing file with a completely unrelated image.
        m_editExistingUrl.clear();
        m_cliOptions[CommandLineOptions::EditExisting] = false;
    }

    // Clear the window title that can be used in file names.
    ExportManager::instance()->setWindowTitle({});

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

    if (PlasmaVersion::get() < PlasmaVersion::check(5, 27, 4) && KX11Extras::compositingActive()) {
        // when compositing is enabled, we need to give it enough time for the window
        // to disappear and all the effects are complete before we take the shot. there's
        // no way of knowing how long the disappearing effects take, but as per default
        // settings (and unless the user has set an extremely slow effect), 200
        // milliseconds is a good amount of wait time.
        timeout = qMax(timeout, 200);
    } else if (m_platform->inherits("PlatformXcb")) {
        // Minimum 50ms delay to prevent segfaults from xcb function calls
        // that don't get replies fast enough.
        timeout = qMax(timeout, 50);
    }

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

void SpectacleCore::showErrorMessage(const QString &message)
{
    qCDebug(SPECTACLE_CORE_LOG) << "ERROR: " << message;

    if (m_startMode == StartMode::Gui) {
        KMessageBox::error(nullptr, message);
    }
}

void SpectacleCore::onScreenshotUpdated(const QImage &image)
{
    m_annotationDocument->setCanvasImages({image});
    ExportManager::instance()->setImage(image);
    ExportManager::instance()->updateTimestamp();

    if (m_startMode == StartMode::Gui) {
        if (image.isNull()) {
            initViewerWindow(ViewerWindow::Dialog);
            ViewerWindow::instance()->setVisible(true);
            return;
        }
        initViewerWindow(ViewerWindow::Image);
        if (m_cliOptions[CommandLineOptions::EditExisting]) {
            ViewerWindow::instance()->setAnnotating(true);
        }
        ViewerWindow::instance()->setVisible(true);
        auto titlePreset = !image.isNull() ? SpectacleWindow::Unsaved : SpectacleWindow::Saved;
        SpectacleWindow::setTitleForAll(titlePreset);
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

void SpectacleCore::doNotify(const ExportManager::Actions &actions, const QUrl &saveUrl)
{
    if (m_cliOptions[CommandLineOptions::NoNotify]) {
        return;
    }

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
    const QString &saveDirPath = saveUrl.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).path();
    const QString &saveFileName = saveUrl.fileName();

    using Action = ExportManager::Action;
    if (actions & Action::AnySave && !saveFileName.isEmpty()) {
        if (actions & Action::CopyPath) {
            notification->setText(i18n("A screenshot was saved as '%1' to '%2' and the file path of the screenshot has been saved to your clipboard.",
                                       saveFileName, saveDirPath));
        } else if (saveDirPath == QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)) {
            notification->setText(i18nc("Placeholder is filename",
                                        "A screenshot was saved as '%1' to your Pictures folder.",
                                        saveFileName));
        } else {
            notification->setText(i18n("A screenshot was saved as '%1' to '%2'.",
                                       saveFileName, saveDirPath));
        }
    } else if (actions & Action::CopyImage) {
        notification->setText(i18n("A screenshot was saved to your clipboard."));
    }

    if (!saveUrl.isEmpty()) {
        notification->setUrls({saveUrl});
        notification->setDefaultAction(i18nc("Open the screenshot we just saved", "Open"));
        connect(notification, &KNotification::defaultActivated, this, [saveUrl]() {
            auto job = new KIO::OpenUrlJob(saveUrl);
            job->start();
        });
        notification->setActions({i18n("Annotate")});
        connect(notification, &KNotification::action1Activated, this, [saveUrl]() {
            QProcess newInstance;
            newInstance.setProgram(QCoreApplication::applicationFilePath());
            newInstance.setArguments({
                CommandLineOptions::toArgument(CommandLineOptions::self()->newInstance),
                CommandLineOptions::toArgument(CommandLineOptions::self()->editExisting),
                saveUrl.toLocalFile()
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

ExportManager::Actions SpectacleCore::autoExportActions() const
{
    using Action = ExportManager::Action;
    using Option = CommandLineOptions::Option;
    bool save = m_startMode != StartMode::Gui && m_cliOptions[Option::Output];
    bool copyImage = m_cliOptions[Option::CopyImage];
    bool copyPath = m_cliOptions[Option::CopyPath];
    ExportManager::Actions actions;
    if (m_startMode != StartMode::Background) {
        save |= Settings::autoSaveImage();
        copyImage |= Settings::clipboardGroup() == Settings::PostScreenshotCopyImage;
        copyPath |= Settings::clipboardGroup() == Settings::PostScreenshotCopyLocation;
    }
    if (m_startMode == StartMode::Gui) {
        actions.setFlag(Action::Save, save);
        actions.setFlag(Action::CopyImage, copyImage);
    } else {
        // In background and dbus mode, ensure that either save or copy image is enabled.
        actions.setFlag(Action::Save, save || !copyImage);
        actions.setFlag(Action::CopyImage, !actions.testFlag(Action::Save) || copyImage);
    }
    actions.setFlag(Action::CopyPath, actions.testFlag(Action::Save) && copyPath);
    return actions;
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

// Hurry up the sync if the sync timer is active.
void SpectacleCore::syncExportImage()
{
    if (!m_annotationSyncTimer->isActive()) {
        return;
    }
    setExportImage(m_annotationDocument->renderToImage());
}

// A convenient way to stop the sync timer and set the export image.
void SpectacleCore::setExportImage(const QImage &image)
{
    m_annotationSyncTimer->stop();
    ExportManager::instance()->setImage(image);
}

QQmlEngine *SpectacleCore::getQmlEngine()
{
    if (m_engine == nullptr) {
        m_engine = std::make_unique<QQmlEngine>(this);
        m_engine->rootContext()->setContextObject(new KLocalizedContext(m_engine.get()));

        qmlRegisterSingletonInstance(QML_URI_PRIVATE, 1, 0, "SpectacleCore", this);
        qmlRegisterSingletonInstance(QML_URI_PRIVATE, 1, 0, "Platform", m_platform.get());
        qmlRegisterSingletonInstance(QML_URI_PRIVATE, 1, 0, "Settings", Settings::self());
        qmlRegisterSingletonInstance(QML_URI_PRIVATE, 1, 0, "CaptureModeModel", m_captureModeModel.get());
        qmlRegisterSingletonInstance(QML_URI_PRIVATE, 1, 0, "SelectionEditor", SelectionEditor::instance());
        qmlRegisterSingletonInstance(QML_URI_PRIVATE, 1, 0, "Selection", SelectionEditor::instance()->selection());
        qmlRegisterSingletonInstance(QML_URI_PRIVATE, 1, 0, "Geometry", Geometry::instance());
        qmlRegisterSingletonInstance(QML_URI_PRIVATE, 1, 0, "G", Geometry::instance());

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

QStringList SpectacleCore::supportedVideoFormats() const
{
    return m_videoPlatform->suggestedExtensions();
}

void SpectacleCore::setVideoFormat(const QString &format)
{
    if (format == Settings::videoFormat()) {
        return;
    }

    m_videoPlatform->setExtension(format);
    Settings::setVideoFormat(m_videoPlatform->extension());

    Q_EMIT videoFormatChanged(format);
}

QString SpectacleCore::videoFormat() const
{
    return m_videoPlatform->extension();
}

#include "moc_SpectacleCore.cpp"
