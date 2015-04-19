/*
 *  Copyright (C) 2015 Boudhayan Gupta <me@BaloneyGeek.com>
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

#ifndef KSCREENGENIEGUI_H
#define KSCREENGENIEGUI_H

#include <QApplication>
#include <QMetaObject>
#include <QVariant>
#include <QList>
#include <QWidget>
#include <QQuickItem>
#include <QQuickWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMoveEvent>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QSharedPointer>
#include <QMenu>
#include <QPoint>
#include <QTimer>
#include <QDebug>
#include <QAction>

#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KAboutData>
#include <KHelpMenu>
#include <KGuiItem>
#include <KStandardGuiItem>
#include <KDeclarative/QmlObject>

#include "ImageGrabber.h"
#include "KSGSendToMenu.h"
#include "KSGImageProvider.h"

class KScreenGenieGUI : public QWidget
{
    Q_OBJECT

    public:

    explicit KScreenGenieGUI(QObject *genie, QWidget *parent = 0);
    ~KScreenGenieGUI();

    void setScreenshotAndShow(const QPixmap &pixmap);

    private slots:

    void captureScreenshot(QString captureMode, double captureDelay, bool includePointer, bool includeDecorations);
    void saveCheckboxStatesConfig(bool includePointer, bool includeDecorations);
    void saveCaptureModeConfig(int modeIndex);
    void ungrabMouseWorkaround();

    signals:

    void newScreenshotRequest(ImageGrabber::GrabMode mode, int timeout, bool includePointer, bool includeDecorations);
    void dragAndDropRequest();
    void saveAndExit();
    void saveAsClicked();
    void sendToKServiceRequest(KService::Ptr servicePointer);
    void sendToClipboardRequest();
    void sendToOpenWithRequest();

    protected:

    void moveEvent(QMoveEvent *event);

    private:

    void init();

    QObject                 *mScreenGenie;
    QQuickWidget            *mQuickWidget;
    QDialogButtonBox        *mDialogButtonBox;
    QPushButton             *mSendToButton;
    KSGSendToMenu           *mSendToMenu;
    KDeclarative::QmlObject *mKQmlObject;
    KSGImageProvider        *mScreenshotImageProvider;
    QList<QAction *>         mMenuActions;
};

#endif // KSCREENGENIEGUI_H
