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
#include <QPrintDialog>
#include <QPrinter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QPushButton>
#include <QToolButton>
#include <QMenu>
#include <QKeySequence>
#include <QDialogButtonBox>
#include <QPoint>
#include <QFrame>
#include <QAction>
#include <QMetaObject>
#include <QTimer>

#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KMessageWidget>
#include <KAboutData>
#include <KHelpMenu>
#include <KGuiItem>
#include <KStandardGuiItem>
#include <KActionCollection>
#include <KStandardAction>

#include "Config.h"
#ifdef XCB_FOUND
#include <QX11Info>
#include <xcb/xcb.h>
#endif

#include "PlatformBackends/ImageGrabber.h"
#include "KSWidget.h"
#include "KSSaveConfigDialog.h"
#include "KSSendToMenu.h"

class KSMainWindow : public QDialog
{
    Q_OBJECT

    public:

    explicit KSMainWindow(bool onClickAvailable, QWidget *parent = 0);
    ~KSMainWindow();

    private slots:

    void captureScreenshot(ImageGrabber::GrabMode mode, int timeout, bool includePointer, bool includeDecorations);
    void showPrintDialog();
    void showSaveConfigDialog();
    void sendToClipboard();
    void init();

    public slots:

    void setScreenshotAndShow(const QPixmap &pixmap);
    void setScreenshotWindowTitle(QUrl location);

    signals:

    void newScreenshotRequest(ImageGrabber::GrabMode mode, int timeout, bool includePointer, bool includeDecorations);
    void dragAndDropRequest();
    void save();
    void saveAndExit();
    void saveAsClicked();
    void sendToKServiceRequest(KService::Ptr servicePointer);
    void sendToClipboardRequest();
    void sendToOpenWithRequest();
    void printRequest(QPrinter *);

    protected:

    void moveEvent(QMoveEvent *event);

    private:

    KSWidget          *mKSWidget;
    QFrame            *mDivider;
    QDialogButtonBox  *mDialogButtonBox;
    QPushButton       *mSendToButton;
    QToolButton       *mClipboardButton;
    QToolButton       *mSaveButton;
    QMenu             *mSaveMenu;
    KMessageWidget    *mCopyMessage;
    KSSendToMenu      *mSendToMenu;
    KActionCollection *mActionCollection;
    bool               mOnClickAvailable;
};

#endif // KSMAINWINDOW_H
