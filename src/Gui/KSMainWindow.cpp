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

#include "KSMainWindow.h"

KSMainWindow::KSMainWindow(bool onClickAvailable, QWidget *parent) :
    QDialog(parent),
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
    // before we do anything, we need to set a window property
    // that skips the close/hide window animation on kwin. this
    // fixes a ghost image of the kapture window that appears
    // on subsequent screenshots taken with the take new screenshot
    // button
    //
    // credits for this goes to Thomas Lübking <thomas.luebking@gmail.com>

#ifdef XCB_FOUND
    if (qApp->platformName() == QStringLiteral("xcb")) {
        // create a window if we haven't already. note that the QWidget constructor
        // should already have done this

        if (winId() == 0) {
            create(0, true, true);
        }

        // do the xcb shenanigans

        xcb_connection_t *xcbConn = QX11Info::connection();
        const QByteArray effectName = QByteArrayLiteral("_KDE_NET_WM_SKIP_CLOSE_ANIMATION");

        xcb_intern_atom_cookie_t atomCookie = xcb_intern_atom_unchecked(xcbConn, false, effectName.length(), effectName.constData());
        QScopedPointer<xcb_intern_atom_reply_t, QScopedPointerPodDeleter> atom(xcb_intern_atom_reply(xcbConn, atomCookie, nullptr));
        if (atom.isNull()) {
          goto done;
        }

        uint32_t value = 1;
        xcb_change_property(xcbConn, XCB_PROP_MODE_REPLACE, winId(), atom->atom, XCB_ATOM_CARDINAL, 32, 1, &value);
    }
#endif

    done:
    QMetaObject::invokeMethod(this, "init", Qt::QueuedConnection);
}

KSMainWindow::~KSMainWindow()
{}

// GUI init

void KSMainWindow::init()
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kapturerc");
    KConfigGroup guiConfig(config, "GuiConfig");

    // window properties

    setMinimumSize(840, 420);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(minimumSize());

    QPoint location = guiConfig.readEntry("window-position", QPoint(50, 50));
    move(location);

    // the KSGWidget

    connect(mKSWidget, &KSWidget::newScreenshotRequest, this, &KSMainWindow::captureScreenshot);
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

    mSaveMenu->addAction(KStandardAction::save(this, SIGNAL(save()), this));
    mSaveMenu->addAction(KStandardAction::saveAs(this, SIGNAL(saveAsClicked()), this));
    mSaveMenu->addAction(KStandardAction::print(this, SLOT(showPrintDialog()), this));
    mSaveMenu->addAction(QIcon::fromTheme("applications-system"), i18n("Configure Save Options"), this, SLOT(showSaveConfigDialog()));

    QAction *saveAndExitAction = new QAction(QIcon::fromTheme("document-save"), i18n("Save &&& Exit"), this);
    saveAndExitAction->setToolTip(i18n("Save screenshot in your Pictures directory and exit"));
    saveAndExitAction->setShortcut(QKeySequence(QKeySequence::Quit));
    connect(saveAndExitAction, &QAction::triggered, this, &KSMainWindow::saveAndExit);

    mSaveButton->setDefaultAction(saveAndExitAction);
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

    KSharedConfigPtr config = KSharedConfig::openConfig("kapturerc");
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
    mKSWidget->setScreenshotPixmap(pixmap);
    setWindowTitle(i18nc("Unsaved Screenshot", "Unsaved[*]"));
    setWindowModified(true);

    KGuiItem::assign(mDialogButtonBox->button(QDialogButtonBox::Discard), KStandardGuiItem::discard());
    show();

    if (mSendToMenu->menu()->isEmpty()) {
        QTimer::singleShot(100, mSendToMenu, &KSSendToMenu::populateMenu);
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
    setWindowModified(false);
    KGuiItem::assign(mDialogButtonBox->button(QDialogButtonBox::Discard), KStandardGuiItem::quit());
}
