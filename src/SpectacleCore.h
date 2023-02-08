/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QQuickItem>
#include <QVariantAnimation>

#include "CaptureModeModel.h"
#include "CommandLineOptions.h"
#include "ExportManager.h"
#include "Gui/Annotations/AnnotationDocument.h"
#include "Gui/CaptureWindow.h"
#include "Gui/ViewerWindow.h"
#include "Platforms/PlatformLoader.h"
#include "RecordingModeModel.h"
#include "settings.h"

#include <memory>

static const auto QML_URI_PRIVATE = "org.kde.spectacle.private";

class SpectacleCore : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Platform *platform READ platform CONSTANT FINAL)
    Q_PROPERTY(CaptureModeModel *captureModeModel READ captureModeModel CONSTANT FINAL)
    Q_PROPERTY(RecordingModeModel *recordingModeModel READ recordingModeModel CONSTANT FINAL)
    Q_PROPERTY(QUrl screenCaptureUrl READ screenCaptureUrl NOTIFY screenCaptureUrlChanged FINAL)
    Q_PROPERTY(int captureTimeRemaining READ captureTimeRemaining NOTIFY captureTimeRemainingChanged FINAL)
    Q_PROPERTY(qreal captureProgress READ captureProgress NOTIFY captureProgressChanged FINAL)
    Q_PROPERTY(bool recordingSupported READ recordingSupported CONSTANT)
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY recordingChanged)
    Q_PROPERTY(QString recordedTime READ recordedTime NOTIFY recordedTimeChanged)
    Q_PROPERTY(bool videoMode READ videoMode NOTIFY videoModeChanged)
    Q_PROPERTY(QUrl currentVideo READ currentVideo NOTIFY currentVideoChanged)
public:
    enum class StartMode {
        Gui = 0,
        DBus = 1,
        Background = 2,
    };

    explicit SpectacleCore(QObject *parent = nullptr);
    ~SpectacleCore() override;

    static SpectacleCore *instance();

    Platform *platform() const;

    CaptureModeModel *captureModeModel() const;
    RecordingModeModel *recordingModeModel() const;

    AnnotationDocument *annotationDocument() const;

    QUrl screenCaptureUrl() const;
    void setScreenCaptureUrl(const QUrl &url);
    // Used when setting the URL from CLI
    void setScreenCaptureUrl(const QString &filePath);

    int captureTimeRemaining() const;
    qreal captureProgress() const;

    void initGuiNoScreenshot();

    void syncExportPixmap();

    void startRecordingWindow(const QString &uuid, bool withPointer);
    void startRecordingRegion(const QRect &region, bool withPointer);
    void startRecordingScreen(QScreen *screen, bool withPointer);
    Q_INVOKABLE void finishRecording();
    bool isRecording() const;
    bool recordingSupported() const;
    bool videoMode() const;
    QUrl currentVideo() const;
    QString recordedTime() const;

public Q_SLOTS:
    void takeNewScreenshot(int captureMode = Settings::captureMode(),
                           int timeout = Settings::captureOnClick() ? -1 : Settings::captureDelay() * 1000,
                           bool includePointer = Settings::includePointer(),
                           bool includeDecorations = Settings::includeDecorations(),
                           bool transientOnly = Settings::transientOnly());
    void cancelScreenshot();
    void showErrorMessage(const QString &theErrString);
    void onScreenshotUpdated(const QPixmap &thePixmap);
    void onScreenshotFailed();
    void doNotify(const QUrl &theSavedAt);

    void activate(QStringList arguments, const QString & /*workingDirectory */);

Q_SIGNALS:
    void screenCaptureUrlChanged();
    void captureTimeRemainingChanged();
    void captureProgressChanged();

    void errorMessage(const QString &errString);
    void allDone();
    void grabDone(const QPixmap &pixmap);
    void grabFailed();
    void recordingChanged(bool isRecording);
    void videoModeChanged(bool videoMode);
    void currentVideoChanged(const QUrl &currentVideo);
    void recordedTimeChanged();

private:
    Platform::GrabMode toPlatformGrabMode(CaptureModeModel::CaptureMode theCaptureMode);
    bool isGuiNull() const;
    QQmlEngine *getQmlEngine();
    void initCaptureWindows(CaptureWindow::Mode mode);
    void initViewerWindow(ViewerWindow::Mode mode);
    void deleteWindows();
    void unityLauncherUpdate(const QVariantMap &properties) const;
    void setVideoMode(bool enabled);
    void setCurrentVideo(const QUrl &currentVideo);

    static SpectacleCore *s_self;
    std::unique_ptr<AnnotationDocument> m_annotationDocument = nullptr;
    StartMode m_startMode = StartMode::Gui;
    bool m_notify = false;
    QUrl m_screenCaptureUrl;
    std::unique_ptr<Platform> m_platform;
    std::unique_ptr<VideoPlatform> m_videoPlatform;
    std::unique_ptr<CaptureModeModel> m_captureModeModel;
    std::unique_ptr<RecordingModeModel> m_recordingModeModel;
    std::unique_ptr<QQmlEngine> m_engine;
    std::unique_ptr<QTimer> m_annotationSyncTimer;
    std::unique_ptr<ViewerWindow> m_viewerWindow;
    // Using std::vector for emplace_back(), needed for appending unique_ptrs to a vector.
    // QVector won't get emplaceBack() until Qt 6.
    std::vector<std::unique_ptr<CaptureWindow>> m_captureWindows;
    std::unique_ptr<QVariantAnimation> m_delayAnimation;

    bool m_copyImageToClipboard = false;
    bool m_copyLocationToClipboard = false;
    bool m_saveToOutput = false;
    bool m_editExisting = false;
    bool m_existingLoaded = false;

    Platform::GrabMode m_tempGrabMode = Platform::GrabMode::InvalidChoice;
    bool m_tempIncludePointer = false;
    bool m_tempIncludeDecorations = false;
    bool m_videoMode = false;
    QUrl m_currentVideo;
};
