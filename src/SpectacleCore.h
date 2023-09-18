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
#include "Gui/Annotations/AnnotationDocument.h"
#include "Gui/CaptureWindow.h"
#include "Gui/ViewerWindow.h"
#include "Platforms/PlatformLoader.h"
#include "RecordingModeModel.h"
#include "settings.h"

#include <array>
#include <memory>

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
    Q_PROPERTY(QStringList supportedVideoFormats READ supportedVideoFormats CONSTANT FINAL)
    Q_PROPERTY(QString videoFormat READ videoFormat WRITE setVideoFormat NOTIFY videoFormatChanged)

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

    QUrl outputUrl() const;

    int captureTimeRemaining() const;
    qreal captureProgress() const;

    void initGuiNoScreenshot();

    void syncExportImage();

    void startRecordingWindow(const QString &uuid, bool withPointer);
    void startRecordingRegion(const QRect &region, bool withPointer);
    void startRecordingScreen(QScreen *screen, bool withPointer);
    Q_INVOKABLE void finishRecording();
    bool isRecording() const;
    bool recordingSupported() const;
    bool videoMode() const;
    QUrl currentVideo() const;
    QString recordedTime() const;
    Q_INVOKABLE QString timeFromMilliseconds(qint64 milliseconds) const;
    QStringList supportedVideoFormats() const;
    void setVideoFormat(const QString &format);
    QString videoFormat() const;

    ExportManager::Actions autoExportActions() const;

public Q_SLOTS:
    void activate(const QStringList &arguments, const QString &workingDirectory);
    void takeNewScreenshot(int captureMode = Settings::captureMode(),
                           int timeout = Settings::captureOnClick() ? -1 : Settings::captureDelay() * 1000,
                           bool includePointer = Settings::includePointer(),
                           bool includeDecorations = Settings::includeDecorations());
    void cancelScreenshot();
    void showErrorMessage(const QString &message);
    void onScreenshotFailed();
    void doNotify(const ExportManager::Actions &actions, const QUrl &saveUrl);

Q_SIGNALS:
    void screenCaptureUrlChanged();
    void captureTimeRemainingChanged();
    void captureProgressChanged();

    void errorMessage(const QString &message);
    void allDone();
    void grabFailed();
    void recordingChanged(bool isRecording);
    void videoModeChanged(bool videoMode);
    void currentVideoChanged(const QUrl &currentVideo);
    void recordedTimeChanged();
    void videoFormatChanged(const QString &format);

private:
    void takeNewScreenshot(Platform::GrabMode grabMode, int timeout,
                           bool includePointer, bool includeDecorations);
    void setExportImage(const QImage &image);
    void showViewerIfGuiMode();
    Platform::GrabMode toGrabMode(CaptureModeModel::CaptureMode captureMode, bool transientOnly) const;
    CaptureModeModel::CaptureMode toCaptureMode(Platform::GrabMode grabMode) const;
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
    QUrl m_screenCaptureUrl;
    std::unique_ptr<Platform> m_platform;
    std::unique_ptr<VideoPlatform> m_videoPlatform;
    std::unique_ptr<CaptureModeModel> m_captureModeModel;
    std::unique_ptr<RecordingModeModel> m_recordingModeModel;
    std::unique_ptr<QQmlEngine> m_engine;
    std::unique_ptr<QTimer> m_annotationSyncTimer;
    std::unique_ptr<QVariantAnimation> m_delayAnimation;
    std::unique_ptr<QEventLoopLocker> m_eventLoopLocker;

    // Use ViewerWindow::instance() to get the viewer window.
    ViewerWindow::UniquePointer m_viewerWindow = {nullptr, nullptr};

    // Use CaptureWindow::instances() to get the capture windows.
    // Don't assume that this will never have entries that are null.
    // For some reason, removeIf/erase_if/find_if then erase doesn't work with QList/QVector,
    // so we have to use std::vector. Something about use of a deleted unique_ptr function.
    std::vector<CaptureWindow::UniquePointer> m_captureWindows;

    bool m_copyImageToClipboard = false;
    bool m_copyLocationToClipboard = false;
    bool m_saveToOutput = false;
    std::array<bool, CommandLineOptions::TotalOptions> m_cliOptions = {};

    QUrl m_editExistingUrl;
    QUrl m_outputUrl;

    Platform::GrabMode m_lastGrabMode = Platform::GrabMode::NoGrabModes;
    bool m_lastIncludePointer = false; // cli default value
    bool m_lastIncludeDecorations = true; // cli default value
    bool m_videoMode = false;
    QUrl m_currentVideo;
};
