/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QEventLoopLocker>
#include <QObject>
#include <QQmlEngine>
#include <QQuickItem>
#include <QVariantAnimation>

#include "CaptureModeModel.h"
#include "CommandLineOptions.h"
#include "ExportManager.h"
#include "Gui/CaptureWindow.h"
#include "Gui/ViewerWindow.h"
#include "OcrManager.h"
#include "Platforms/PlatformLoader.h"
#include "RecordingModeModel.h"
#include "VideoFormatModel.h"
#include "settings.h"

#include <KQuickImageEditor/AnnotationDocument>

#include <array>
#include <memory>

class SpectacleCore : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(ImagePlatform *imagePlatform READ imagePlatform CONSTANT FINAL)
    Q_PROPERTY(VideoPlatform *videoPlatform READ videoPlatform CONSTANT FINAL)
    Q_PROPERTY(QUrl screenCaptureUrl READ screenCaptureUrl NOTIFY screenCaptureUrlChanged FINAL)
    Q_PROPERTY(int captureTimeRemaining READ captureTimeRemaining NOTIFY captureTimeRemainingChanged FINAL)
    Q_PROPERTY(qreal captureProgress READ captureProgress NOTIFY captureProgressChanged FINAL)
    Q_PROPERTY(QString recordedTime READ recordedTime NOTIFY recordedTimeChanged)
    Q_PROPERTY(bool videoMode READ videoMode WRITE setVideoMode NOTIFY videoModeChanged)
    Q_PROPERTY(QUrl currentVideo READ currentVideo NOTIFY currentVideoChanged)
    Q_PROPERTY(AnnotationDocument *annotationDocument READ annotationDocument CONSTANT FINAL)
    Q_PROPERTY(bool ocrAvailable READ ocrAvailable NOTIFY ocrStatusChanged FINAL)
    Q_PROPERTY(OcrManager::OcrStatus ocrStatus READ ocrStatus NOTIFY ocrStatusChanged FINAL)

public:
    enum class StartMode {
        Gui = 0,
        DBus = 1,
        Background = 2,
    };

    ~SpectacleCore() noexcept override;

    static SpectacleCore *instance();

    ImagePlatform *imagePlatform() const;
    VideoPlatform *videoPlatform() const;

    AnnotationDocument *annotationDocument() const;

    QUrl screenCaptureUrl() const;
    void setScreenCaptureUrl(const QUrl &url);
    // Used when setting the URL from CLI
    void setScreenCaptureUrl(const QString &filePath);

    QUrl outputUrl() const;

    int captureTimeRemaining() const;
    qreal captureProgress() const;

    QString recordedTime() const;

    bool videoMode() const;
    void setVideoMode(bool enabled);

    QUrl currentVideo() const;

    bool ocrAvailable() const;
    OcrManager::OcrStatus ocrStatus() const;
    Q_INVOKABLE QVariantMap ocrAvailableLanguages() const;
    Q_INVOKABLE bool startOcrExtraction(const QString &languageCode = QString());

    void initGuiNoScreenshot();

    void syncExportImage();

    Q_INVOKABLE void startRecording(VideoPlatform::RecordingMode mode, bool withPointer = Settings::videoIncludePointer());
    Q_INVOKABLE void finishRecording();

    Q_INVOKABLE QString timeFromMilliseconds(qint64 milliseconds) const;

    ExportManager::Actions autoExportActions() const;

    void activateAction(const QString &actionName, const QVariant &parameter);

    static SpectacleCore *create(QQmlEngine *engine, QJSEngine *)
    {
        auto inst = instance();
        Q_ASSERT(inst);
        Q_ASSERT(inst->thread() == engine->thread());

        if (s_qmlEngine) {
            Q_ASSERT(engine == s_qmlEngine);
        } else {
            s_qmlEngine = engine;
        }

        QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
        return inst;
    }

public Q_SLOTS:
    void activate(const QStringList &arguments, const QString &workingDirectory);
    void takeNewScreenshot(int captureMode = Settings::captureMode(),
                           int timeout = Settings::captureOnClick() ? -1 : Settings::captureDelay() * 1000 * Settings::captureDelayEnabled(),
                           bool includePointer = Settings::includePointer(),
                           bool includeDecorations = Settings::includeDecorations(),
                           bool includeShadow = Settings::includeShadow());
    void cancelScreenshot();
    void showErrorMessage(const QString &message);

Q_SIGNALS:
    void screenCaptureUrlChanged();
    void captureTimeRemainingChanged();
    void captureProgressChanged();

    void allDone();
    void dbusScreenshotFailed(const QString &message);
    void dbusRecordingFailed(const QString &message);
    void videoModeChanged(bool videoMode);
    void currentVideoChanged(const QUrl &currentVideo);
    void recordedTimeChanged();
    void ocrStatusChanged();

private:
    explicit SpectacleCore(QObject *parent = nullptr);

    enum class ScreenCapture {
        Screenshot,
        Recording,
    };

    void takeNewScreenshot(ImagePlatform::GrabMode grabMode, int timeout, bool includePointer, bool includeDecorations, bool includeShadow);
    void setExportImage(const QImage &image);
    void showViewerIfGuiMode(bool minimized = false);
    void doNotify(ScreenCapture type, const ExportManager::Actions &actions, const QUrl &saveUrl);
    ImagePlatform::GrabMode toGrabMode(CaptureModeModel::CaptureMode captureMode, bool transientOnly) const;
    CaptureModeModel::CaptureMode toCaptureMode(ImagePlatform::GrabMode grabMode) const;
    bool isGuiNull() const;
    QQmlEngine *getQmlEngine();
    void initCaptureWindows(CaptureWindow::Mode mode);
    void initViewerWindow(ViewerWindow::Mode mode);
    void deleteWindows();
    void unityLauncherUpdate(const QVariantMap &properties) const;
    void setCurrentVideo(const QUrl &currentVideo);
    QUrl videoOutputUrl() const;
    bool performOcrExtraction(const QString &languageCode);

    static SpectacleCore *s_self;
    std::unique_ptr<AnnotationDocument> m_annotationDocument = nullptr;
    StartMode m_startMode = StartMode::Gui;
    bool m_returnToViewer = false;
    bool m_ocrExportInProgress = false;
    bool m_quitAfterOcr = false;
    QUrl m_screenCaptureUrl;
    std::unique_ptr<ImagePlatform> m_imagePlatform;
    std::unique_ptr<VideoPlatform> m_videoPlatform;
    std::unique_ptr<QQmlEngine> m_engine;
    std::unique_ptr<QTimer> m_annotationSyncTimer;
    std::unique_ptr<QVariantAnimation> m_delayAnimation;
    std::unique_ptr<QEventLoopLocker> m_eventLoopLocker;

    // Use ViewerWindow::instance() to get the viewer window.
    ViewerWindow::UniquePointer m_viewerWindow = {nullptr, nullptr};

    // Use CaptureWindow::instances() to get the capture windows.
    // Don't assume that this will never have entries that are null.
    // For some reason, removeIf/erase_if/find_if then erase doesn't work with QList/QList,
    // so we have to use std::vector. Something about use of a deleted unique_ptr function.
    std::vector<CaptureWindow::UniquePointer> m_captureWindows;

    std::array<bool, CommandLineOptions::TotalOptions> m_cliOptions = {};

    QUrl m_editExistingUrl;
    QUrl m_outputUrl;

    ImagePlatform::GrabMode m_lastGrabMode = ImagePlatform::GrabMode::NoGrabModes;
    bool m_lastIncludePointer = false; // cli default value
    bool m_lastIncludeDecorations = true; // cli default value
    bool m_lastIncludeShadow = true; // cli default value
    VideoPlatform::RecordingMode m_lastRecordingMode = VideoPlatform::NoRecordingModes;
    bool m_videoMode = false;
    QUrl m_currentVideo;

    static inline QQmlEngine *s_qmlEngine = nullptr;
};
