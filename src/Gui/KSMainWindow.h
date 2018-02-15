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

#ifndef KSMAINWINDOW_H
#define KSMAINWINDOW_H

#include <QDialog>
#include <QMenu>
#include <QFrame>
#include <QToolButton>
#include <QDialogButtonBox>

#include <KMessageWidget>

#include "PlatformBackends/ImageGrabber.h"
#include "ExportMenu.h"
#include "KSWidget.h"
#include "SpectacleConfig.h"

class KMoreToolsMenuFactory;

class KSMainWindow : public QDialog
{
    Q_OBJECT

    public:

    explicit KSMainWindow(bool onClickAvailable, QWidget *parent = 0);
    ~KSMainWindow();

    private:

    SaveMode saveButtonMode() const;

    private slots:

    void captureScreenshot(ImageGrabber::GrabMode mode, int timeout, bool includePointer, bool includeDecorations);
    void showPrintDialog();
    void openScreenshotsFolder();
    void showPreferencesDialog();
    void showImageSharedFeedback(bool error, const QString &message);
    void sendToClipboard();
    void init();
    void setDefaultSaveAction();
    void save();
    void saveAs();
    int windowWidth(const QPixmap &pixmap) const;

    public slots:

    void setScreenshotAndShow(const QPixmap &pixmap);
    void setScreenshotWindowTitle(QUrl location);

    signals:

    void newScreenshotRequest(ImageGrabber::GrabMode mode, int timeout, bool includePointer, bool includeDecorations);
    void dragAndDropRequest();

    protected:

    void moveEvent(QMoveEvent *event) Q_DECL_OVERRIDE;

    private:

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
    KMessageWidget   *mMessageWidget;
    QMenu            *mToolsMenu;
    QMenu            *mScreenRecorderToolsMenu;
    QScopedPointer<KMoreToolsMenuFactory> mScreenrecorderToolsMenuFactory;
    ExportMenu       *mExportMenu;
    bool              mOnClickAvailable;
};

#endif // KSMAINWINDOW_H
