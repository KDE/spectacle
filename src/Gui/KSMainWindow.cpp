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

#include "KSMainWindow.h"

KSMainWindow::KSMainWindow(bool onClickAvailable, QWidget *parent) :
    QWidget(parent),
    mDialogButtonBox(nullptr),
    mSendToButton(nullptr),
    mPrintButton(nullptr),
    mSendToMenu(new KSSendToMenu),
    mOnClickAvailable(onClickAvailable)
{
    QMetaObject::invokeMethod(this, "init", Qt::QueuedConnection);
}

KSMainWindow::~KSMainWindow()
{}

// GUI init

void KSMainWindow::init()
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kscreengenierc");
    KConfigGroup guiConfig(config, "GuiConfig");

    // window properties

    setWindowTitle(i18nc("Unsaved Screenshot", "Unsaved"));
    setFixedSize(800, 370);

    QPoint location = guiConfig.readEntry("window-position", QPoint(50, 50));
    move(location);

    // the KSGWidget

    mKSWidget = new KSWidget(this);

    connect(mKSWidget, &KSWidget::newScreenshotRequest, this, &KSMainWindow::captureScreenshot);
    connect(mKSWidget, &KSWidget::checkboxStatesChanged, this, &KSMainWindow::saveCheckboxStatesConfig);
    connect(mKSWidget, &KSWidget::captureModeChanged, this, &KSMainWindow::saveCaptureModeConfig);
    connect(mKSWidget, &KSWidget::dragInitiated, this, &KSMainWindow::dragAndDropRequest);

    // the Button Bar

    mDialogButtonBox = new QDialogButtonBox(this);
    mDialogButtonBox->setStandardButtons(QDialogButtonBox::Help | QDialogButtonBox::Discard | QDialogButtonBox::Apply | QDialogButtonBox::Save);

    mSendToButton = new QPushButton;
    KGuiItem::assign(mSendToButton, KGuiItem(i18n("Send To...")));
    mDialogButtonBox->addButton(mSendToButton, QDialogButtonBox::ActionRole);

    mPrintButton = new QPushButton;
    KGuiItem::assign(mPrintButton, KStandardGuiItem::print());
    connect(mPrintButton, &QPushButton::clicked, this, &KSMainWindow::showPrintDialog);
    mDialogButtonBox->addButton(mPrintButton, QDialogButtonBox::ActionRole);

    connect(mDialogButtonBox->button(QDialogButtonBox::Discard), &QPushButton::clicked, qApp, &QApplication::quit);
    connect(mDialogButtonBox->button(QDialogButtonBox::Save), &QPushButton::clicked, this, &KSMainWindow::saveAsClicked);
    KGuiItem::assign(mDialogButtonBox->button(QDialogButtonBox::Save), KStandardGuiItem::saveAs());

    connect(mDialogButtonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &KSMainWindow::saveAndExit);
    KGuiItem::assign(mDialogButtonBox->button(QDialogButtonBox::Apply), KStandardGuiItem::save());
    mDialogButtonBox->button(QDialogButtonBox::Apply)->setText(i18n("Save && Exit"));
    mDialogButtonBox->button(QDialogButtonBox::Apply)->setToolTip(i18n("Quicksave screenshot in your Pictures directory and exit"));

    KHelpMenu *helpMenu = new KHelpMenu(this, KAboutData::applicationData(), true);
    mDialogButtonBox->button(QDialogButtonBox::Help)->setMenu(helpMenu->menu());

    // layouts

    mDivider = new QFrame(this);
    mDivider->setFrameShape(QFrame::HLine);
    mDivider->setLineWidth(2);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(mKSWidget);
    layout->addWidget(mDivider);
    layout->addWidget(mDialogButtonBox);

    // populate our send-to actions

    connect(mSendToMenu, &KSSendToMenu::sendToServiceRequest, this, &KSMainWindow::sendToKServiceRequest);
    connect(mSendToMenu, &KSSendToMenu::sendToClipboardRequest, this, &KSMainWindow::sendToClipboardRequest);
    connect(mSendToMenu, &KSSendToMenu::sendToOpenWithRequest, this, &KSMainWindow::sendToOpenWithRequest);

    mSendToButton->setMenu(mSendToMenu->menu());

    // read in the checkbox states and capture mode index

    bool capturePointer = guiConfig.readEntry("includePointer", true);
    bool captureDecorations = guiConfig.readEntry("includeDecorations", true);
    bool captureOnClick = guiConfig.readEntry("waitCaptureOnClick", false);
    mKSWidget->setCheckboxStates(capturePointer, captureDecorations, captureOnClick);

    int captureModeIndex = guiConfig.readEntry("captureModeIndex", 0);
    mKSWidget->setCaptureModeIndex(captureModeIndex);

    // disable onClick mode if not available on the platform

    if (!mOnClickAvailable) {
        mKSWidget->disableOnClick();
    }

    // done with the init
}

// overrides

void KSMainWindow::moveEvent(QMoveEvent *event)
{
    Q_UNUSED(event);

    KSharedConfigPtr config = KSharedConfig::openConfig("kscreengenierc");
    KConfigGroup guiConfig(config, "GuiConfig");

    guiConfig.writeEntry("window-position", pos());
    guiConfig.sync();
}

// slots

void KSMainWindow::captureScreenshot(ImageGrabber::GrabMode mode, int timeout, bool includePointer, bool includeDecorations)
{
    hide();
    emit newScreenshotRequest(mode, timeout, includePointer, includeDecorations);
}

void KSMainWindow::setScreenshotAndShow(const QPixmap &pixmap)
{
    show();
    mKSWidget->setScreenshotPixmap(pixmap);

    if (mSendToMenu->menu()->isEmpty()) {
        QTimer::singleShot(50, mSendToMenu, &KSSendToMenu::populateMenu);
    }
}

void KSMainWindow::showPrintDialog()
{
    QPrinter *printer = new QPrinter(QPrinter::HighResolution);
    QPrintDialog printDialog(printer, this);
    if (printDialog.exec() == QDialog::Accepted) {
        emit printRequest(printer);
        return;
    }
    delete printer;
}

void KSMainWindow::setScreenshotWindowTitle(QUrl location)
{
    setWindowTitle(location.fileName());
}

void KSMainWindow::saveCheckboxStatesConfig(bool includePointer, bool includeDecorations, bool waitCaptureOnClick)
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kscreengenierc");
    KConfigGroup guiConfig(config, "GuiConfig");

    guiConfig.writeEntry("includePointer", includePointer);
    guiConfig.writeEntry("includeDecorations", includeDecorations);
    guiConfig.writeEntry("waitCaptureOnClick", waitCaptureOnClick);
    guiConfig.sync();
}

void KSMainWindow::saveCaptureModeConfig(int modeIndex)
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kscreengenierc");
    KConfigGroup guiConfig(config, "GuiConfig");

    guiConfig.writeEntry("captureModeIndex", modeIndex);
    guiConfig.sync();
}
