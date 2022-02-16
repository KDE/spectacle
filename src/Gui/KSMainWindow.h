/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QDialog>
class QDialogButtonBox;
class QFrame;
class QMenu;
class QStackedLayout;
class QToolButton;

class KMessageWidget;
#include <KNS3/KMoreToolsMenuFactory>

#include "SpectacleCommon.h"

#include "Config.h"
#include "ExportMenu.h"
#include "KSWidget.h"
#include "Platforms/Platform.h"

#include <KMessageWidget>
#include <memory>

class KSMainWindow : public QDialog
{
    Q_OBJECT

public:
    explicit KSMainWindow(Platform::GrabModes theGrabModes, Platform::ShutterModes theShutterModes, QWidget *parent = nullptr);
    ~KSMainWindow() override = default;

    enum class MessageDuration { AutoHide, Persistent };

private:
    enum class QuitBehavior { QuitImmediately, QuitExternally };
    void quit(const QuitBehavior quitBehavior = QuitBehavior::QuitImmediately);
    void showInlineMessage(const QString &message,
                           const KMessageWidget::MessageType messageType,
                           const MessageDuration messageDuration = MessageDuration::AutoHide,
                           const QList<QAction *> &actions = {});
    int windowWidth(const QPixmap &pixmap) const;

private Q_SLOTS:

    void captureScreenshot(Spectacle::CaptureMode theCaptureMode, int theTimeout, bool theIncludePointer, bool theIncludeDecorations);
    void showPrintDialog();
    void openScreenshotsFolder();
    void showPreferencesDialog();
    void showImageSharedFeedback(bool error, const QString &message);
    void imageCopied();
    void imageSavedAndLocationCopied(const QUrl &location);
    void init();
    void setDefaultSaveAction();
    void setDefaultCopyAction();
    void save();
    void saveAs();
    void copyImage();
    void copyLocation();
    void restoreWindowTitle();

public Q_SLOTS:

    void setScreenshotAndShow(const QPixmap &pixmap, bool showAnnotator);
    void imageSaved(const QUrl &location);
    void imageSavedAndCopied(const QUrl &location);
    void screenshotFailed();
    void setPlaceholderTextOnLaunch();

Q_SIGNALS:

    void newScreenshotRequest(Spectacle::CaptureMode theCaptureMode, int theTimeout, bool theIncludePointer, bool theIncludeDecorations);
    void dragAndDropRequest();

protected:
    void moveEvent(QMoveEvent *event) override;
    QSize sizeHint() const override;

private:
    void keyPressEvent(QKeyEvent *event) override;

    KSWidget *const mKSWidget;
    QFrame *const mDivider;
    QDialogButtonBox *const mDialogButtonBox;
    QToolButton *const mConfigureButton;
    QPushButton *const mToolsButton;
    QPushButton *const mSendToButton;
    QToolButton *const mClipboardButton;
    QMenu *const mClipboardMenu;
    QAction *mClipboardLocationAction = nullptr;
    QAction *mClipboardImageAction = nullptr;
    QToolButton *const mSaveButton;
    QMenu *const mSaveMenu;
    QAction *mSaveAsAction = nullptr;
    QAction *mSaveAction = nullptr;
    QAction *mOpenContaining = nullptr;
    KMessageWidget *const mMessageWidget;
    QMenu *const mToolsMenu;
    QMenu *mScreenRecorderToolsMenu = nullptr;
    std::unique_ptr<KMoreToolsMenuFactory> mScreenrecorderToolsMenuFactory;
    ExportMenu *const mExportMenu;
    Platform::ShutterModes mShutterModes;
    QTimer *mHideMessageWidgetTimer = nullptr;
    QStackedLayout *mStack = nullptr;
    bool mPixmapExists = false;

#ifdef KIMAGEANNOTATOR_FOUND
    QToolButton *const mAnnotateButton;
    bool mAnnotatorActive;
#endif
};
