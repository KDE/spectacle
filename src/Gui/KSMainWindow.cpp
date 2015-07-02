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
    mKSWidget(new KSWidget),
    mDivider(new QFrame),
    mDialogButtonBox(new QDialogButtonBox),
    mSendToButton(new QPushButton),
    mClipboardButton(new QToolButton),
    mSaveButton(new QToolButton),
    mSaveMenu(new QMenu),
    mCopyMessage(new KMessageWidget),
    mSendToMenu(new KSSendToMenu),
    mActionCollection(new KActionCollection(this, "KSStandardActions")),
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
    setMinimumSize(800, 385);
    resize(minimumSize());

    QPoint location = guiConfig.readEntry("window-position", QPoint(50, 50));
    move(location);

    // the KSGWidget

    connect(mKSWidget, &KSWidget::newScreenshotRequest, this, &KSMainWindow::captureScreenshot);
    connect(mKSWidget, &KSWidget::checkboxStatesChanged, this, &KSMainWindow::saveCheckboxStatesConfig);
    connect(mKSWidget, &KSWidget::captureModeChanged, this, &KSMainWindow::saveCaptureModeConfig);
    connect(mKSWidget, &KSWidget::dragInitiated, this, &KSMainWindow::dragAndDropRequest);

    // the Button Bar

    mDialogButtonBox->setStandardButtons(QDialogButtonBox::Help | QDialogButtonBox::Discard);

    KGuiItem::assign(mSendToButton, KGuiItem(i18n("Open With...")));
    mSendToButton->setIcon(QIcon::fromTheme("application-x-executable"));
    mDialogButtonBox->addButton(mSendToButton, QDialogButtonBox::ActionRole);

    mClipboardButton->setDefaultAction(KStandardAction::copy(this, SLOT(sendToClipboard()), this));
    mClipboardButton->setText(i18n("Copy To Clipboard"));
    mClipboardButton->setToolTip(i18n("Copy the current screenshot image to the clipboard."));
    mClipboardButton->setIcon(QIcon::fromTheme("edit-copy"));
    mClipboardButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mDialogButtonBox->addButton(mClipboardButton, QDialogButtonBox::ActionRole);

    mSaveMenu->addAction(KStandardAction::saveAs(this, SIGNAL(saveAsClicked()), this));
    mSaveMenu->addAction(KStandardAction::print(this, SLOT(showPrintDialog()), this));
    mSaveMenu->addAction(QIcon::fromTheme("applications-system"), i18n("Configure Save Options"), this, SLOT(showSaveConfigDialog()));

    mSaveButton->setDefaultAction(KStandardAction::save(this, SIGNAL(saveAndExit()), this));
    mSaveButton->setText(i18n("Save && Exit"));
    mSaveButton->setToolTip(i18n("Save screenshot in your Pictures directory and exit"));
    mSaveButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mSaveButton->setMenu(mSaveMenu);
    mSaveButton->setPopupMode(QToolButton::MenuButtonPopup);
    mDialogButtonBox->addButton(mSaveButton, QDialogButtonBox::ActionRole);

    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), mDialogButtonBox->button(QDialogButtonBox::Discard));
    connect(shortcut, &QShortcut::activated, qApp, &QApplication::quit);
    connect(mDialogButtonBox->button(QDialogButtonBox::Discard), &QPushButton::clicked, qApp, &QApplication::quit);

    // the help menu

    KHelpMenu *helpMenu = new KHelpMenu(this, KAboutData::applicationData(), true);
    mDialogButtonBox->button(QDialogButtonBox::Help)->setMenu(helpMenu->menu());

    // copy-to-clipboard message

    mCopyMessage->setText(i18n("The screenshot has been copied to the clipboard."));
    mCopyMessage->setMessageType(KMessageWidget::Information);
    mCopyMessage->setIcon(QIcon::fromTheme("dialog-information"));

    // layouts

    mDivider->setFrameShape(QFrame::HLine);
    mDivider->setLineWidth(2);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(mKSWidget);
    layout->addWidget(mCopyMessage);
    layout->addWidget(mDivider);
    layout->addWidget(mDialogButtonBox);
    mCopyMessage->hide();

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

void KSMainWindow::sendToClipboard()
{
    emit sendToClipboardRequest();
    mCopyMessage->animatedShow();
    QTimer::singleShot(5000, mCopyMessage, &KMessageWidget::animatedHide);
}

void KSMainWindow::showSaveConfigDialog()
{
    KSSaveConfigDialog saveDialog(this);
    saveDialog.exec();
}

void KSMainWindow::setScreenshotWindowTitle(QUrl location)
{
    setWindowTitle(location.fileName());
    KGuiItem::assign(mDialogButtonBox->button(QDialogButtonBox::Discard), KStandardGuiItem::quit());
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
