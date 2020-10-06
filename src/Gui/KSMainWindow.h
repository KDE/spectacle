/* This file is part of Spectacle, the KDE screenshot utility
 * Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QDialog>
#include <QMenu>
#include <QFrame>
#include <QToolButton>
#include <QDialogButtonBox>
#include <QStackedLayout>

#include <KMessageWidget>
#include <KNS3/KMoreToolsMenuFactory>

#include "SpectacleCommon.h"

#include "KSWidget.h"
#include "ExportMenu.h"
#include "Platforms/Platform.h"
#include "Config.h"

#include <memory>

class KSMainWindow: public QDialog
{
    Q_OBJECT

    public:

    explicit KSMainWindow(Platform::GrabModes theGrabModes, Platform::ShutterModes theShutterModes, QWidget *parent = nullptr);
    virtual ~KSMainWindow() = default;

    enum class MessageDuration {
        AutoHide,
        Persistent
    };

    private:

    enum class QuitBehavior {
        QuitImmediately,
        QuitExternally
    };
    void quit(const QuitBehavior quitBehavior = QuitBehavior::QuitImmediately);
    void showInlineMessage(const QString& message,
                        const KMessageWidget::MessageType messageType,
                        const MessageDuration messageDuration = MessageDuration::AutoHide,
                        const QList<QAction*>& actions  = {});
    int windowWidth(const QPixmap &pixmap) const;

    private Q_SLOTS:

    void captureScreenshot(Spectacle::CaptureMode theCaptureMode, int theTimeout, bool theIncludePointer, bool theIncludeDecorations);
    void showPrintDialog();
    void openScreenshotsFolder();
    void showPreferencesDialog();
    void showImageSharedFeedback(bool error, const QString &message);
    void imageCopied();
    void init();
    void setDefaultSaveAction();
    void save();
    void saveAs();
    void restoreWindowTitle();

    public Q_SLOTS:

    void setScreenshotAndShow(const QPixmap &pixmap);
    void imageSaved(const QUrl &location);
    void imageSavedAndCopied(const QUrl &location);
    void screenshotFailed();

    Q_SIGNALS:

    void newScreenshotRequest(Spectacle::CaptureMode theCaptureMode, int theTimeout, bool theIncludePointer, bool theIncludeDecorations);
    void dragAndDropRequest();

    protected:

    void moveEvent(QMoveEvent *event) override;

    private:

    void keyPressEvent(QKeyEvent *event) override;
    void copy();

    KSWidget         *mKSWidget;
    QFrame           *mDivider;
    QDialogButtonBox *mDialogButtonBox;
    QToolButton      *mConfigureButton;
    QPushButton      *mToolsButton;
    QPushButton      *mSendToButton;
    QToolButton      *mClipboardButton;
    QToolButton      *mSaveButton;
    QMenu            *mSaveMenu;
    QAction          *mSaveAsAction;
    QAction          *mSaveAction;
    QAction          *mOpenContaining;
    KMessageWidget   *mMessageWidget;
    QMenu            *mToolsMenu;
    QMenu            *mScreenRecorderToolsMenu;
    std::unique_ptr<KMoreToolsMenuFactory> mScreenrecorderToolsMenuFactory;
    ExportMenu       *mExportMenu;
    Platform::ShutterModes mShutterModes;
    QTimer           *mHideMessageWidgetTimer;
    QStackedLayout   *mStack;

#ifdef KIMAGEANNOTATOR_FOUND
    QToolButton      *mAnnotateButton;
    bool             mAnnotatorActive;
#endif
};
