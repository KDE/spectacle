/*
 *  SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SpectacleCore.h"
#include "CaptureModeModel.h"
#include "CommandLineOptions.h"
#include "ExportManager.h"
#include "Geometry.h"
#include "Gui/Annotations/AnnotationViewport.h"
#include "Gui/CaptureWindow.h"
#include "Gui/Selection.h"
#include "Gui/SelectionEditor.h"
#include "Gui/SpectacleWindow.h"
#include "Gui/ExportMenu.h"
#include "Gui/HelpMenu.h"
#include "Gui/OptionsMenu.h"
#include "Platforms/VideoPlatform.h"
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
#include <LayerShellQt/Shell>
#include <LayerShellQt/Window>

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
#include <QSystemTrayIcon>
#include <QTimer>
#include <QtMath>
#include <qobjectdefs.h>
#include <utility>

using namespace Qt::StringLiterals;

SpectacleCore *SpectacleCore::s_self = nullptr;
static std::unique_ptr<QSystemTrayIcon> s_systemTrayIcon;

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
        {u"progress-visible"_s, false},
        {u"progress"_s, 0}
    });
    using State = QVariantAnimation::State;
    auto onStateChanged = [this](State newState, State oldState) {
        Q_UNUSED(oldState)
        if (newState == State::Running) {
            unityLauncherUpdate({{u"progress-visible"_s, true}});
        } else if (newState == State::Stopped) {
            unityLauncherUpdate({{u"progress-visible"_s, false}});
            m_delayAnimation->setCurrentTime(0);
        }
    };
    auto onValueChanged = [this](const QVariant &value) {
        Q_EMIT captureTimeRemainingChanged();
        Q_EMIT captureProgressChanged();
        unityLauncherUpdate({{u"progress"_s, value.toReal()}});
        const auto windows = SpectacleWindow::instances();
        if (m_delayAnimation->state() != State::Stopped && !windows.isEmpty()) {
            if (captureTimeRemaining() <= 500 && windows.constFirst()->isVisible()) {
                SpectacleWindow::setVisibilityForAll(QWindow::Hidden);
            }
            SpectacleWindow::setTitleForAll(SpectacleWindow::Timer);
        }
    };
    auto onFinished = [this]() {
        m_imagePlatform->doGrab(ImagePlatform::ShutterMode::Immediate, m_lastGrabMode, m_lastIncludePointer, m_lastIncludeDecorations, m_lastIncludeShadow);
    };
    QObject::connect(delayAnimation, &QVariantAnimation::stateChanged,
                     this, onStateChanged, Qt::QueuedConnection);
    QObject::connect(delayAnimation, &QVariantAnimation::valueChanged,
                     this, onValueChanged, Qt::QueuedConnection);
    QObject::connect(delayAnimation, &QVariantAnimation::finished,
                     this, onFinished, Qt::QueuedConnection);

    m_imagePlatform = loadImagePlatform();
    m_videoPlatform = loadVideoPlatform();
    auto imagePlatform = m_imagePlatform.get();
    m_annotationDocument = std::make_unique<AnnotationDocument>(new AnnotationDocument(this));

    // essential connections
    connect(SelectionEditor::instance(), &SelectionEditor::accepted,
            this, [this](const QRectF &rect, const ExportManager::Actions &actions){
        ExportManager::instance()->updateTimestamp();
        if (m_videoMode) {
            const auto captureWindows = CaptureWindow::instances();
            SpectacleWindow::setVisibilityForAll(QWindow::Hidden);
            for (auto captureWindow : captureWindows) {
                // Destroy the QPlatformWindow so we can change the window behavior.
                // The QPlatformWindow will be recreated when the window is shown again.
                captureWindow->destroy();
                captureWindow->setFlag(Qt::WindowTransparentForInput, true);
                captureWindow->setFlag(Qt::WindowStaysOnTopHint, true);
                if (auto window = LayerShellQt::Window::get(captureWindow)) {
                    using namespace LayerShellQt;
                    window->setCloseOnDismissed(true);
                    window->setLayer(Window::LayerOverlay);
                    auto anchors = Window::Anchors::fromInt(Window::AnchorTop | Window::AnchorBottom | Window::AnchorLeft | Window::AnchorRight);
                    window->setAnchors(anchors);
                    window->setKeyboardInteractivity(Window::KeyboardInteractivityNone);
                }
            }
            SpectacleWindow::setVisibilityForAll(QWindow::FullScreen);
            // deleteWindows();
            // showViewerIfGuiMode(true);
            bool includePointer = m_cliOptions[CommandLineOptions::Pointer];
            includePointer |= m_startMode != StartMode::Background && Settings::videoIncludePointer();
            const auto &output = m_outputUrl.isLocalFile() ? videoOutputUrl() : QUrl();
            m_videoPlatform->startRecording(output, VideoPlatform::Region, rect.toRect(), includePointer);
        } else {
            deleteWindows();
            m_annotationDocument->cropCanvas(rect);
            syncExportImage();
            showViewerIfGuiMode();
            SpectacleWindow::setTitleForAll(SpectacleWindow::Unsaved);
            const auto &exportActions = actions & ExportManager::AnyAction ? actions : autoExportActions();
            ExportManager::instance()->exportImage(exportActions, outputUrl());
        }
    });

    connect(imagePlatform, &ImagePlatform::newScreenshotTaken, this, [this](const QImage &image){
        m_annotationDocument->clearAnnotations();
        m_annotationDocument->setImage(image);
        setExportImage(image);
        ExportManager::instance()->updateTimestamp();
        showViewerIfGuiMode();
        SpectacleWindow::setTitleForAll(SpectacleWindow::Unsaved);
        ExportManager::instance()->exportImage(autoExportActions(), outputUrl());
        setVideoMode(false);
    });
    connect(imagePlatform, &ImagePlatform::newCroppableScreenshotTaken, this, [this](const QImage &image) {
        m_annotationDocument->clearAnnotations();
        m_annotationDocument->setImage(image);
        SelectionEditor::instance()->reset();

        initCaptureWindows(CaptureWindow::Image);
        SpectacleWindow::setTitleForAll(SpectacleWindow::Unsaved);
        SpectacleWindow::setVisibilityForAll(QWindow::FullScreen);
    });
    connect(imagePlatform, &ImagePlatform::newScreenshotFailed, this, &SpectacleCore::onScreenshotFailed);

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
                doNotify(ScreenCapture::Screenshot, actions, url);
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
                viewerWindow->showSavedMessage(url);
            }
        } else if (actions & ExportManager::CopyImage) {
            viewerWindow->showCopiedMessage();
        }
    };
    connect(exportManager, &ExportManager::imageExported, this, onImageExported);
    auto onVideoExported = [this](const ExportManager::Actions &actions, const QUrl &url) {
        setCurrentVideo(url);

        if (actions & ExportManager::UserAction && Settings::quitAfterSaveCopyExport()) {
            deleteWindows();
        } else if (!ViewerWindow::instance()) {
            showViewerIfGuiMode();
        }

        if (isGuiNull()) {
            if (m_cliOptions[CommandLineOptions::NoNotify]) {
                Q_EMIT allDone();
            } else {
                doNotify(ScreenCapture::Recording, actions, url);
            }
            return;
        }

        auto viewerWindow = ViewerWindow::instance();
        if (!viewerWindow) {
            return;
        }

        if (actions & ExportManager::AnySave) {
            SpectacleWindow::setTitleForAll(SpectacleWindow::Saved, url.fileName());
            if (actions & ExportManager::CopyPath) {
                viewerWindow->showSavedAndLocationCopiedMessage(url, true);
            } else {
                viewerWindow->showSavedMessage(url, true);
            }
        } else if (actions & ExportManager::CopyPath) {
            viewerWindow->showLocationCopiedMessage();
        }
    };
    connect(exportManager, &ExportManager::videoExported, this, onVideoExported);
    connect(exportManager, &ExportManager::errorMessage, this, &SpectacleCore::showErrorMessage);

    connect(imagePlatform, &ImagePlatform::windowTitleChanged, exportManager, &ExportManager::setWindowTitle);
    connect(m_annotationDocument.get(), &AnnotationDocument::repaintNeeded, m_annotationSyncTimer.get(), qOverload<>(&QTimer::start));
    connect(m_annotationSyncTimer.get(), &QTimer::timeout, this, [this] {
        ExportManager::instance()->setImage(m_annotationDocument->renderToImage());
    }, Qt::QueuedConnection); // QueuedConnection to help prevent making the visible render lag.

    // set up shortcuts
    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->openAction(),
                                            QList<QKeySequence>{
                                                Qt::Key_Print,
                                                // Default screenshot shortcut on Windows.
                                                // Also for keyboards without a print screen key.
                                                Qt::META | Qt::SHIFT | Qt::Key_S,
                                            });
    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->fullScreenAction(), Qt::SHIFT | Qt::Key_Print);
    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->activeWindowAction(), Qt::META | Qt::Key_Print);
    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->windowUnderCursorAction(), Qt::META | Qt::CTRL | Qt::Key_Print);
    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->regionAction(), Qt::META | Qt::SHIFT | Qt::Key_Print);
    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->currentScreenAction(), QList<QKeySequence>());
    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->openWithoutScreenshotAction(), QList<QKeySequence>());
    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->recordScreenAction(), Qt::META | Qt::ALT | Qt::Key_R);
    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->recordWindowAction(), Qt::META | Qt::CTRL | Qt::Key_R);
    KGlobalAccel::self()->setGlobalShortcut(ShortcutActions::self()->recordRegionAction(),
                                            QList<QKeySequence>{
                                                // Similar to region screenshot
                                                Qt::META | Qt::SHIFT | Qt::Key_R,
                                                // Also use Meta+R for now
                                                Qt::META | Qt::Key_R,
                                            });

    // set up CaptureMode model
    m_captureModeModel = std::make_unique<CaptureModeModel>(imagePlatform->supportedGrabModes(), this);
    m_recordingModeModel = std::make_unique<RecordingModeModel>(m_videoPlatform->supportedRecordingModes(), this);
    m_videoFormatModel = std::make_unique<VideoFormatModel>(m_videoPlatform->supportedFormats(), this);
    auto captureModeModel = m_captureModeModel.get();
    connect(imagePlatform, &ImagePlatform::supportedGrabModesChanged, captureModeModel, [this](){
        m_captureModeModel->setGrabModes(m_imagePlatform->supportedGrabModes());
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

    s_systemTrayIcon = std::make_unique<QSystemTrayIcon>(QIcon::fromTheme(u"media-record-symbolic"_s));
    auto systemTrayIcon = s_systemTrayIcon.get();
    connect(systemTrayIcon, &QSystemTrayIcon::activated, systemTrayIcon, [](auto reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            SpectacleCore::instance()->finishRecording();
        }
    });

    auto videoPlatform = m_videoPlatform.get();
    connect(videoPlatform, &VideoPlatform::recordingChanged,
            systemTrayIcon, [this](bool isRecording){
        s_systemTrayIcon->setVisible(isRecording);
        if (!isRecording) {
            m_captureWindows.clear();
        }
    });
    connect(videoPlatform, &VideoPlatform::recordedTimeChanged, this, [this] {
        Q_EMIT recordedTimeChanged();
        s_systemTrayIcon->setToolTip(i18nc("@info:tooltip", "Spectacle is recording: %1\nClick to finish recording", recordedTime()));
    });
    connect(videoPlatform, &VideoPlatform::recordingSaved, this, [this](const QUrl &fileUrl) {
        // Always try to save. Needed to move recordings out of temp dir.
        ExportManager::instance()->exportVideo(autoExportActions() | ExportManager::Save, fileUrl, videoOutputUrl());
    });
    connect(videoPlatform, &VideoPlatform::recordingCanceled, this, [this] {
        if (m_startMode != StartMode::Gui || isGuiNull()) {
            Q_EMIT allDone();
            return;
        }
        SpectacleWindow::setTitleForAll(SpectacleWindow::Previous);
    });
    connect(videoPlatform, &VideoPlatform::recordingFailed, this, [this](const QString &message){
        switch (m_startMode) {
        case StartMode::Background:
            if (!message.isEmpty()) {
                showErrorMessage(message);
            }
            Q_EMIT allDone();
            return;
        case StartMode::DBus:
            Q_EMIT dbusRecordingFailed();
            Q_EMIT allDone();
            return;
        case StartMode::Gui:
            if (!ViewerWindow::instance()) {
                initViewerWindow(ViewerWindow::Dialog);
            }
            ViewerWindow::instance()->showRecordingFailedMessage(message);
            return;
        }
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

ImagePlatform *SpectacleCore::imagePlatform() const
{
    return m_imagePlatform.get();
}

CaptureModeModel *SpectacleCore::captureModeModel() const
{
    return m_captureModeModel.get();
}

RecordingModeModel *SpectacleCore::recordingModeModel() const
{
    return m_recordingModeModel.get();
}

VideoFormatModel *SpectacleCore::videoFormatModel() const
{
    return m_videoFormatModel.get();
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
    parser.parse(arguments);

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
    if (!(Settings::rememberSelectionRect() == Settings::EnumRememberSelectionRect::Always)) {
        Settings::setSelectionRect({0, 0, 0, 0});
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
    bool includeShadow;
    if (m_startMode == StartMode::Background) {
        transientOnly = m_cliOptions[Option::TransientOnly];
        onClick = m_cliOptions[Option::OnClick];
        includeDecorations = !m_cliOptions[Option::NoDecoration];
        includePointer = m_cliOptions[Option::Pointer];
        includeShadow = !m_cliOptions[Option::NoShadow];
    } else {
        transientOnly = Settings::transientOnly() || m_cliOptions[Option::TransientOnly];
        onClick = Settings::captureOnClick() || m_cliOptions[Option::OnClick];
        includeDecorations = Settings::includeDecorations()
                            && !m_cliOptions[Option::NoDecoration];
        includeShadow = Settings::includeShadow() && !m_cliOptions[Option::NoShadow];
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
            // If editing an existing image, open the annotation editor.
            // This QImage constructor only works with local files or Qt resource file names.
            QImage existingImage(existingLocalFile);
            m_annotationDocument->clearAnnotations();
            m_annotationDocument->setImage(existingImage);
            showViewerIfGuiMode();
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
    using GrabMode = ImagePlatform::GrabMode;
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

    using RecordingMode = VideoPlatform::RecordingMode;
    RecordingMode recordingMode = RecordingMode::NoRecordingModes;
    if (m_cliOptions[Option::Record]) {
        auto input = parser.value(CommandLineOptions::self()->record);
        if (input.startsWith(u"s"_s, Qt::CaseInsensitive)) {
            recordingMode = RecordingMode::Screen;
        } else if (input.startsWith(u"w"_s, Qt::CaseInsensitive)) {
            recordingMode = RecordingMode::Window;
        } else if (input.startsWith(u"r"_s, Qt::CaseInsensitive)) {
            recordingMode = RecordingMode::Region;
        } else {
            // QCommandLineParser handles the case where input is empty
            qWarning().noquote() << i18nc("@info:shell", "%1 is not a valid mode for --record", input);
            Q_EMIT allDone();
            return;
        }
        setVideoMode(true);

        if (m_startMode != StartMode::Background) {
            includePointer = Settings::videoIncludePointer() || m_cliOptions[Option::Pointer];
        }
    } else {
        setVideoMode(false);
    }

    // If any capture mode is given in the cli options, let it override
    // the setting to not take a screenshot on launch
    // clang-format off
    bool captureModeFromCli =
        m_cliOptions[Option::Fullscreen] ||
        m_cliOptions[Option::Current] ||
        m_cliOptions[Option::ActiveWindow] ||
        m_cliOptions[Option::WindowUnderCursor] ||
        m_cliOptions[Option::TransientOnly] ||
        m_cliOptions[Option::Region] ||
        m_cliOptions[Option::Record];
    // clang-format on

    switch (m_startMode) {
    case StartMode::DBus:
        break;
    case StartMode::Background:
        if (m_videoMode) {
            startRecording(recordingMode, includePointer);
        } else {
            takeNewScreenshot(grabMode, delayMsec, includePointer, includeDecorations, includeShadow);
        }
        break;
    case StartMode::Gui:
        if (isGuiNull()) {
            if (m_cliOptions[Option::LaunchOnly] || //
                (Settings::launchAction() == Settings::DoNotTakeScreenshot && !captureModeFromCli)) {
                initViewerWindow(ViewerWindow::Dialog);
                ViewerWindow::instance()->setVisible(true);
            } else {
                if (m_videoMode) {
                    startRecording(recordingMode, includePointer);
                } else {
                    takeNewScreenshot(grabMode, delayMsec, includePointer, includeDecorations, includeShadow);
                }
            }
        } else {
            using Actions = Settings::EnumPrintKeyRunningAction;
            switch (Settings::printKeyRunningAction()) {
            case Actions::TakeNewScreenshot: {
                // takeNewScreenshot switches to on click if immediate is not supported.
                takeNewScreenshot(grabMode, 0, includePointer, includeDecorations, includeShadow);
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

void SpectacleCore::takeNewScreenshot(ImagePlatform::GrabMode grabMode, int timeout, bool includePointer, bool includeDecorations, bool includeWindowShadow)
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
    m_lastIncludeShadow = includeWindowShadow;

    if ((timeout < 0 || !m_imagePlatform->supportedShutterModes().testFlag(ImagePlatform::Immediate))
        && m_imagePlatform->supportedShutterModes().testFlag(ImagePlatform::OnClick)
    ) {
        SpectacleWindow::setVisibilityForAll(QWindow::Hidden);
        m_imagePlatform->doGrab(ImagePlatform::ShutterMode::OnClick, m_lastGrabMode, m_lastIncludePointer, m_lastIncludeDecorations, m_lastIncludeShadow);
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
    } else if (m_imagePlatform->inherits("PlatformXcb")) {
        // Minimum 50ms delay to prevent segfaults from xcb function calls
        // that don't get replies fast enough.
        timeout = qMax(timeout, 50);
    }

    if (noDelay) {
        SpectacleWindow::setVisibilityForAll(QWindow::Hidden);
        QTimer::singleShot(timeout, this, [this]() {
            m_imagePlatform->doGrab(ImagePlatform::ShutterMode::Immediate, m_lastGrabMode, m_lastIncludePointer, m_lastIncludeDecorations, m_lastIncludeShadow);
        });
        return;
    }

    m_delayAnimation->setDuration(timeout);
    m_delayAnimation->start();

    SpectacleWindow::setVisibilityForAll(QWindow::Minimized);
}

void SpectacleCore::takeNewScreenshot(int captureMode, int timeout, bool includePointer, bool includeDecorations, bool includeShadow)
{
    using CaptureMode = CaptureModeModel::CaptureMode;
    takeNewScreenshot(toGrabMode(CaptureMode(captureMode), Settings::transientOnly()), timeout, includePointer, includeDecorations, includeShadow);
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

void SpectacleCore::showViewerIfGuiMode(bool minimized)
{
    if (m_startMode != StartMode::Gui) {
        return;
    }
    initViewerWindow(ViewerWindow::Image);
    if (!m_videoMode && m_cliOptions[CommandLineOptions::EditExisting]) {
        ViewerWindow::instance()->setAnnotating(true);
    }
    if (minimized) {
        ViewerWindow::instance()->showMinimized();
    } else {
        ViewerWindow::instance()->setVisible(true);
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
        Q_EMIT dbusScreenshotFailed();
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

static QList<KNotification *> notifications;

void SpectacleCore::doNotify(ScreenCapture type, const ExportManager::Actions &actions, const QUrl &saveUrl)
{
    if (m_cliOptions[CommandLineOptions::NoNotify]) {
        return;
    }

    // ensure program stays alive until the notification finishes.
    if (!m_eventLoopLocker) {
        m_eventLoopLocker = std::make_unique<QEventLoopLocker>();
    }

    KNotification *notification = nullptr;
    QString title;
    if (type == ScreenCapture::Screenshot) {
        notification = new KNotification(u"newScreenshotSaved"_s, KNotification::CloseOnTimeout, this);
        int index = captureModeModel()->indexOfCaptureMode(toCaptureMode(m_lastGrabMode));
        title = captureModeModel()->data(captureModeModel()->index(index), Qt::DisplayRole).toString();
    } else {
        notification = new KNotification(u"recordingSaved"_s, KNotification::CloseOnTimeout, this);
        int index = m_recordingModeModel->indexOfRecordingMode(m_lastRecordingMode);
        title = m_recordingModeModel->data(m_recordingModeModel->index(index), Qt::DisplayRole).toString();
    }
    notification->setTitle(title);

    notifications.append(notification);

    // a speaking message is prettier than a URL, special case for copy image/location to clipboard and the default pictures location
    const QString &saveDirPath = saveUrl.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).path();
    const QString &saveFileName = saveUrl.fileName();

    using Action = ExportManager::Action;
    if (type == ScreenCapture::Screenshot) {
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
    } else if (type == ScreenCapture::Recording && actions & Action::AnySave && !saveFileName.isEmpty()) {
        if (actions & Action::CopyPath) {
            notification->setText(
                i18n("A recording was saved as '%1' to '%2' and the file path of the recording has been saved to your clipboard.", saveFileName, saveDirPath));
        } else if (saveDirPath == QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)) {
            notification->setText(i18nc("Placeholder is filename", "A recording was saved as '%1' to your Videos folder.", saveFileName));
        } else {
            notification->setText(i18n("A recording was saved as '%1' to '%2'.", saveFileName, saveDirPath));
        }
    }

    if (!saveUrl.isEmpty()) {
        notification->setUrls({saveUrl});

        auto open = [saveUrl]() {
            auto job = new KIO::OpenUrlJob(saveUrl);
            job->start();
        };
        auto defaultAction = notification->addDefaultAction(i18nc("Open the screenshot we just saved", "Open"));
        connect(defaultAction, &KNotificationAction::activated, this, open);

        if (type == ScreenCapture::Screenshot) {
            auto annotate = [saveUrl]() {
                QProcess newInstance;
                newInstance.setProgram(QCoreApplication::applicationFilePath());
                newInstance.setArguments({
                    CommandLineOptions::toArgument(CommandLineOptions::self()->newInstance),
                    CommandLineOptions::toArgument(CommandLineOptions::self()->editExisting),
                    saveUrl.toLocalFile()
                });
                newInstance.startDetached();
            };
            auto annotateAction = notification->addAction(i18n("Annotate"));
            connect(annotateAction, &KNotificationAction::activated, this, annotate);
        }
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
    bool save = (m_startMode != StartMode::Gui && m_cliOptions[Option::Output]) || m_videoMode;
    bool copyImage = m_cliOptions[Option::CopyImage] && !m_videoMode;
    bool copyPath = m_cliOptions[Option::CopyPath];
    ExportManager::Actions actions;
    if (m_startMode != StartMode::Background) {
        save |= Settings::autoSaveImage();
        copyImage |= Settings::clipboardGroup() == Settings::PostScreenshotCopyImage && !m_videoMode;
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

ImagePlatform::GrabMode SpectacleCore::toGrabMode(CaptureModeModel::CaptureMode captureMode, bool transientOnly) const
{
    using GrabMode = ImagePlatform::GrabMode;
    using CaptureMode = CaptureModeModel::CaptureMode;
    const auto &supportedGrabModes = m_imagePlatform->supportedGrabModes();
    if (captureMode == CaptureMode::CurrentScreen
        && supportedGrabModes.testFlag(ImagePlatform::CurrentScreen)) {
        return GrabMode::CurrentScreen;
    } else if (captureMode == CaptureMode::ActiveWindow
        && supportedGrabModes.testFlag(ImagePlatform::ActiveWindow)) {
        return GrabMode::ActiveWindow;
    } else if (captureMode == CaptureMode::WindowUnderCursor
        && supportedGrabModes.testFlag(ImagePlatform::WindowUnderCursor)) {
        // TODO: Improve API for transientOnly or make it obsolete.
        if (transientOnly || !supportedGrabModes.testFlag(ImagePlatform::TransientWithParent)) {
            return GrabMode::WindowUnderCursor;
        } else {
            return GrabMode::TransientWithParent;
        }
    } else if (captureMode == CaptureMode::RectangularRegion
        && supportedGrabModes.testFlag(ImagePlatform::PerScreenImageNative)) {
        return GrabMode::PerScreenImageNative;
    } else if (captureMode == CaptureMode::AllScreensScaled
        && supportedGrabModes.testFlag(ImagePlatform::AllScreensScaled)) {
        return GrabMode::AllScreensScaled;
    } else if (supportedGrabModes.testFlag(ImagePlatform::AllScreens)) { // default if supported
        return GrabMode::AllScreens;
    } else {
        return GrabMode::NoGrabModes;
    }
}

CaptureModeModel::CaptureMode SpectacleCore::toCaptureMode(ImagePlatform::GrabMode grabMode) const
{
    using GrabMode = ImagePlatform::GrabMode;
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

        qmlRegisterSingletonInstance(SPECTACLE_QML_URI, 1, 0, "SpectacleCore", this);
        qmlRegisterSingletonInstance(SPECTACLE_QML_URI, 1, 0, "ImagePlatform", m_imagePlatform.get());
        qmlRegisterSingletonInstance(SPECTACLE_QML_URI, 1, 0, "VideoPlatform", m_videoPlatform.get());
        qmlRegisterSingletonInstance(SPECTACLE_QML_URI, 1, 0, "Settings", Settings::self());
        qmlRegisterSingletonInstance(SPECTACLE_QML_URI, 1, 0, "CaptureModeModel", m_captureModeModel.get());
        qmlRegisterSingletonInstance(SPECTACLE_QML_URI, 1, 0, "SelectionEditor", SelectionEditor::instance());
        qmlRegisterSingletonInstance(SPECTACLE_QML_URI, 1, 0, "Selection", SelectionEditor::instance()->selection());
        qmlRegisterSingletonInstance(SPECTACLE_QML_URI, 1, 0, "Geometry", Geometry::instance());
        qmlRegisterSingletonInstance(SPECTACLE_QML_URI, 1, 0, "G", Geometry::instance());
        qmlRegisterSingletonInstance(SPECTACLE_QML_URI, 1, 0, "ExportMenu", ExportMenu::instance());
        qmlRegisterSingletonInstance(SPECTACLE_QML_URI, 1, 0, "HelpMenu", HelpMenu::instance());
        qmlRegisterSingletonInstance(SPECTACLE_QML_URI, 1, 0, "OptionsMenu", OptionsMenu::instance());

        qmlRegisterSingletonInstance(SPECTACLE_QML_URI, 1, 0, "AnnotationDocument", m_annotationDocument.get());
        qmlRegisterUncreatableType<AnnotationTool>(SPECTACLE_QML_URI, 1, 0, "AnnotationTool",
                                                   u"Use AnnotationDocument.tool"_s);
        qmlRegisterUncreatableType<SelectedActionWrapper>(SPECTACLE_QML_URI, 1, 0, "SelectedAction",
                                                          u"Use AnnotationDocument.selectedAction"_s);
        qmlRegisterType<AnnotationViewport>(SPECTACLE_QML_URI, 1, 0, "AnnotationViewport");
        qmlRegisterUncreatableType<QScreen>(SPECTACLE_QML_URI, 1, 0, "QScreen",
                                            u"Only created by Qt"_s);
    }
    return m_engine.get();
}

void SpectacleCore::initCaptureWindows(CaptureWindow::Mode mode)
{
    deleteWindows();

    if (mode == CaptureWindow::Video) {
        LayerShellQt::Shell::useLayerShell();
    }

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
    QDBusMessage message = QDBusMessage::createSignal(u"/org/kde/Spectacle"_s,
                                                      u"com.canonical.Unity.LauncherEntry"_s,
                                                      u"Update"_s);
    message.setArguments({QApplication::desktopFileName(), properties});
    QDBusConnection::sessionBus().send(message);
}

void SpectacleCore::startRecording(VideoPlatform::RecordingMode mode, bool withPointer)
{
    if (m_videoPlatform->isRecording() || mode == VideoPlatform::NoRecordingModes) {
        return;
    }
    m_lastRecordingMode = mode;
    setVideoMode(true);
    if (mode == VideoPlatform::Region) {
        SelectionEditor::instance()->reset();
        initCaptureWindows(CaptureWindow::Video);
        SpectacleWindow::setTitleForAll(SpectacleWindow::Unsaved);
        SpectacleWindow::setVisibilityForAll(QWindow::FullScreen);
    } else {
        const auto &output = m_outputUrl.isLocalFile() ? videoOutputUrl() : QUrl();
        m_videoPlatform->startRecording(output, mode, {}, withPointer);
    }
}

void SpectacleCore::finishRecording()
{
    Q_ASSERT(m_videoPlatform->isRecording());
    m_videoPlatform->finishRecording();
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

QUrl SpectacleCore::videoOutputUrl() const
{
    return VideoPlatform::formatForPath(m_outputUrl.path()) != VideoPlatform::NoFormat ? m_outputUrl : QUrl();
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

void SpectacleCore::activateAction(const QString &actionName, const QVariant &parameter)
{
    Q_UNUSED(parameter)
    if (actionName == ShortcutActions::self()->fullScreenAction()->objectName()) {
        takeNewScreenshot(CaptureModeModel::AllScreens, 0);
    } else if (actionName == ShortcutActions::self()->currentScreenAction()->objectName()) {
        takeNewScreenshot(CaptureModeModel::CurrentScreen, 0);
    } else if (actionName == ShortcutActions::self()->activeWindowAction()->objectName()) {
        takeNewScreenshot(CaptureModeModel::ActiveWindow, 0);
    } else if (actionName == ShortcutActions::self()->windowUnderCursorAction()->objectName()) {
        takeNewScreenshot(CaptureModeModel::WindowUnderCursor, 0);
    } else if (actionName == ShortcutActions::self()->regionAction()->objectName()) {
        takeNewScreenshot(CaptureModeModel::RectangularRegion, 0);
    } else if (actionName == ShortcutActions::self()->recordRegionAction()->objectName()) {
        if (!m_videoPlatform->isRecording()) {
            startRecording(VideoPlatform::Region);
        } else {
            finishRecording();
        }
    } else if (actionName == ShortcutActions::self()->recordScreenAction()->objectName()) {
        if (!m_videoPlatform->isRecording()) {
            startRecording(VideoPlatform::Screen);
        } else {
            finishRecording();
        }
    } else if (actionName == ShortcutActions::self()->recordWindowAction()->objectName()) {
        if (!m_videoPlatform->isRecording()) {
            startRecording(VideoPlatform::Window);
        } else {
            finishRecording();
        }
    } else if (actionName == ShortcutActions::self()->openWithoutScreenshotAction()->objectName()) {
        initGuiNoScreenshot();
    }
}

#include "moc_SpectacleCore.cpp"
