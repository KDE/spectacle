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
#include "Config.h"

#include <QJsonArray>
#include <QPrintDialog>
#ifdef XCB_FOUND
#include <QX11Info>
#include <xcb/xcb.h>
#endif

#include <KGuiItem>
#include <KStandardAction>
#include <KStandardGuiItem>
#include <KHelpMenu>
#include <KAboutData>
#include <Purpose/AlternativesModel>
#include <PurposeWidgets/Menu>

#include "KSSaveConfigDialog.h"
#include "ExportMenu.h"
#include "ExportManager.h"

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
    mExportMenu(new ExportMenu(this)),
    mShareMenu(new Purpose::Menu(this)),
    mOnClickAvailable(onClickAvailable)
{
    // before we do anything, we need to set a window property
    // that skips the close/hide window animation on kwin. this
    // fixes a ghost image of the spectacle window that appears
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
    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("spectaclerc"));
    KConfigGroup guiConfig(config, "GuiConfig");

    // window properties

    setMinimumSize(840, 420);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(minimumSize());

    QPoint location = guiConfig.readEntry("window-position", QPoint(50, 50));
    move(location);

    // change window title on save

    connect(ExportManager::instance(), &ExportManager::imageSaved, this, &KSMainWindow::setScreenshotWindowTitle);

    // the KSGWidget

    connect(mKSWidget, &KSWidget::newScreenshotRequest, this, &KSMainWindow::captureScreenshot);
    connect(mKSWidget, &KSWidget::dragInitiated, this, &KSMainWindow::dragAndDropRequest);

    // the Button Bar

    mDialogButtonBox->setStandardButtons(QDialogButtonBox::Help | QDialogButtonBox::Discard);

    KGuiItem::assign(mSendToButton, KGuiItem(i18n("Export To...")));
    mSendToButton->setIcon(QIcon::fromTheme(QStringLiteral("application-x-executable")));
    mDialogButtonBox->addButton(mSendToButton, QDialogButtonBox::ActionRole);

    mClipboardButton->setDefaultAction(KStandardAction::copy(this, SLOT(sendToClipboard()), this));
    mClipboardButton->setText(i18n("Copy To Clipboard"));
    mClipboardButton->setToolTip(i18n("Copy the current screenshot image to the clipboard."));
    mClipboardButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    mClipboardButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mDialogButtonBox->addButton(mClipboardButton, QDialogButtonBox::ActionRole);

    mSaveMenu->addAction(KStandardAction::save(this, SLOT(save()), this));
    mSaveMenu->addAction(KStandardAction::saveAs(this, SLOT(saveAs()), this));
    mSaveMenu->addAction(KStandardAction::print(this, SLOT(showPrintDialog()), this));
    mSaveMenu->addAction(QIcon::fromTheme(QStringLiteral("applications-system")), i18n("Configure Save Options"), this, SLOT(showSaveConfigDialog()));

    QAction *saveAndExitAction = new QAction(QIcon::fromTheme(QStringLiteral("document-save")), i18n("Save &&& Exit"), this);
    saveAndExitAction->setToolTip(i18n("Save screenshot in your Pictures directory and exit"));
    saveAndExitAction->setShortcut(QKeySequence(QKeySequence::Quit));
    connect(saveAndExitAction, &QAction::triggered, this, &KSMainWindow::saveAndExit);

    mSaveButton->setDefaultAction(saveAndExitAction);
    mSaveButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mSaveButton->setMenu(mSaveMenu);
    mSaveButton->setPopupMode(QToolButton::MenuButtonPopup);
    mDialogButtonBox->addButton(mSaveButton, QDialogButtonBox::ActionRole);

    connect(mShareMenu, &Purpose::Menu::finished, this, [this](const QJsonObject &output, int error, const QString &message) {
        mCopyMessage->setIcon(QIcon::fromTheme(QStringLiteral("dialog-information")));
        if (error==0) {
            mCopyMessage->setMessageType(KMessageWidget::Information);
            mCopyMessage->setText(i18n("<qt>You can find the share picture at:<br /><a href='%1'>%1</a> </qt>", output["url"].toString()));
        } else {
            mCopyMessage->setMessageType(KMessageWidget::Error);
            mCopyMessage->setText(i18n("Couldn't export the patch.\n%1", message));
        }
        mCopyMessage->animatedShow();
    });
    mShareMenu->model()->setInputData(QJsonObject {
        { QStringLiteral("mimeType"), QStringLiteral("image/png") },
        { QStringLiteral("urls"), {} }
    });
    mShareMenu->model()->setPluginType("Export");

    QPushButton* shareButton = new QPushButton(i18n("Share..."));
    shareButton->setMenu(mShareMenu);
    mDialogButtonBox->addButton(shareButton, QDialogButtonBox::ActionRole);

    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), mDialogButtonBox->button(QDialogButtonBox::Discard));
    connect(shortcut, &QShortcut::activated, qApp, &QApplication::quit);
    connect(mDialogButtonBox->button(QDialogButtonBox::Discard), &QPushButton::clicked, qApp, &QApplication::quit);

    // the help menu

    KHelpMenu *helpMenu = new KHelpMenu(this, KAboutData::applicationData(), true);
    mDialogButtonBox->button(QDialogButtonBox::Help)->setMenu(helpMenu->menu());

    // copy-to-clipboard message

    mCopyMessage->setText(i18n("The screenshot has been copied to the clipboard."));
    mCopyMessage->setMessageType(KMessageWidget::Information);
    mCopyMessage->setIcon(QIcon::fromTheme(QStringLiteral("dialog-information")));

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

    mSendToButton->setMenu(mExportMenu);

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

    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("spectaclerc"));
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

    mShareMenu->model()->setInputData(QJsonObject {
        { QStringLiteral("mimeType"), QStringLiteral("image/png") },
        { QStringLiteral("urls"), QJsonArray{ExportManager::instance()->pixmapUrl()} }
    });
}

void KSMainWindow::showPrintDialog()
{
    QPrinter *printer = new QPrinter(QPrinter::HighResolution);
    QPrintDialog printDialog(printer, this);
    if (printDialog.exec() == QDialog::Accepted) {
        ExportManager::instance()->doPrint(printer);
        return;
    }
    delete printer;
}

void KSMainWindow::sendToClipboard()
{
    ExportManager::instance()->doCopyToClipboard();
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

void KSMainWindow::save()
{
    ExportManager::instance()->doSave();
}

void KSMainWindow::saveAs()
{
    ExportManager::instance()->doSaveAs(this);
}

void KSMainWindow::saveAndExit()
{
    ExportManager::instance()->doSave();
    QApplication::quit();
}
