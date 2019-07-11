/* This file is part of Spectacle, the KDE screenshot utility
 * Copyright 2019 David Redondo <kde@david-redondo.de>
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

#include "KSMainWindow.h"

#include "Config.h"
#include "SettingsDialog/SettingsDialog.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QPrintDialog>
#include <QPushButton>
#include <QTimer>
#include <QtMath>
#include <QVariantAnimation>
#include <QVBoxLayout>

#ifdef XCB_FOUND
#include <QX11Info>
#include <xcb/xcb.h>
#endif

#include <KAboutData>
#include <KGuiItem>
#include <KHelpMenu>
#include <KIO/OpenFileManagerWindowJob>
#include <KLocalizedString>
#include <KStandardAction>
#include <KWindowSystem>

static const int DEFAULT_WINDOW_HEIGHT = 420;
static const int DEFAULT_WINDOW_WIDTH = 840;
static const int MAXIMUM_WINDOW_WIDTH = 1000;

KSMainWindow::KSMainWindow(const Platform::GrabModes &theGrabModes, const Platform::ShutterModes &theShutterModes, QWidget *parent) :
    QDialog(parent),
    mKSWidget(new KSWidget(theGrabModes, this)),
    mDivider(new QFrame(this)),
    mDialogButtonBox(new QDialogButtonBox(this)),
    mConfigureButton(new QToolButton(this)),
    mToolsButton(new QPushButton(this)),
    mSendToButton(new QPushButton(this)),
    mClipboardButton(new QToolButton(this)),
    mSaveButton(new QToolButton(this)),
    mSaveMenu(new QMenu(this)),
    mSaveAsAction(new QAction(this)),
    mSaveAction(new QAction(this)),
    mMessageWidget(new KMessageWidget(this)),
    mToolsMenu(new QMenu(this)),
    mScreenRecorderToolsMenu(new QMenu(this)),
    mExportMenu(new ExportMenu(this)),
    mShutterModes(theShutterModes)
{
    // before we do anything, we need to set a window property
    // that skips the close/hide window animation on kwin. this
    // fixes a ghost image of the spectacle window that appears
    // on subsequent screenshots taken with the take new screenshot
    // button
    //
    // credits for this goes to Thomas Lübking <thomas.luebking@gmail.com>

#ifdef XCB_FOUND
    if (KWindowSystem::isPlatformX11()) {
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
    done:
#endif

    QMetaObject::invokeMethod(this, "init", Qt::QueuedConnection);
}

// GUI init

void KSMainWindow::init()
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("spectaclerc"));
    KConfigGroup guiConfig(config, "GuiConfig");

    // window properties
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QPoint location = guiConfig.readEntry("window-position", QPoint(50, 50));
    move(location);

    // change window title on save

    connect(ExportManager::instance(), &ExportManager::imageSaved, this, &KSMainWindow::imageSaved);

    // the KSGWidget

    connect(mKSWidget, &KSWidget::newScreenshotRequest, this, &KSMainWindow::captureScreenshot);
    connect(mKSWidget, &KSWidget::dragInitiated, this, &KSMainWindow::dragAndDropRequest);

    // the Button Bar

    mDialogButtonBox->setStandardButtons(QDialogButtonBox::Help);

    mConfigureButton->setDefaultAction(KStandardAction::preferences(this, SLOT(showPreferencesDialog()), this));
    mConfigureButton->setText(i18n("Configure..."));
    mConfigureButton->setToolTip(i18n("Change Spectacle's settings."));
    mConfigureButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mDialogButtonBox->addButton(mConfigureButton, QDialogButtonBox::ResetRole);

    KGuiItem::assign(mToolsButton, KGuiItem(i18n("Tools")));
    mToolsButton->setIcon(QIcon::fromTheme(QStringLiteral("tools"),
                                           QIcon::fromTheme(QStringLiteral("application-menu"))));
    mDialogButtonBox->addButton(mToolsButton, QDialogButtonBox::ActionRole);
    mToolsButton->setMenu(mToolsMenu);

    KGuiItem::assign(mSendToButton, KGuiItem(i18n("Export")));
    mSendToButton->setIcon(QIcon::fromTheme(QStringLiteral("document-share")));
    mDialogButtonBox->addButton(mSendToButton, QDialogButtonBox::ActionRole);

    mClipboardButton->setDefaultAction(KStandardAction::copy(this, SLOT(sendToClipboard()), this));
    mClipboardButton->setText(i18n("Copy to Clipboard"));
    mClipboardButton->setToolTip(i18n("Copy the current screenshot image to the clipboard."));
    mClipboardButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mDialogButtonBox->addButton(mClipboardButton, QDialogButtonBox::ActionRole);

    mSaveButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mSaveButton->setMenu(mSaveMenu);
    mSaveButton->setPopupMode(QToolButton::MenuButtonPopup);
    mDialogButtonBox->addButton(mSaveButton, QDialogButtonBox::ActionRole);

    // the help menu
    KHelpMenu *helpMenu = new KHelpMenu(this, KAboutData::applicationData(), true);
    mDialogButtonBox->button(QDialogButtonBox::Help)->setMenu(helpMenu->menu());

    // the tools menu
    mToolsMenu->addAction(QIcon::fromTheme(QStringLiteral("document-open-folder")),
                          i18n("Open Screenshots Folder"),
                          this, &KSMainWindow::openScreenshotsFolder);
    mToolsMenu->addAction(KStandardAction::print(this, &KSMainWindow::showPrintDialog, this));
    mScreenRecorderToolsMenu = mToolsMenu->addMenu(i18n("Record Screen"));
    mScreenRecorderToolsMenu->setIcon(QIcon::fromTheme(QStringLiteral("media-record")));
    connect(mScreenRecorderToolsMenu, &QMenu::aboutToShow, this, [this]()
    {
        KMoreToolsMenuFactory *moreToolsMenuFactory = new KMoreToolsMenuFactory(QStringLiteral("spectacle/screenrecorder-tools"));
        moreToolsMenuFactory->setParentWidget(this);
        mScreenrecorderToolsMenuFactory.reset(moreToolsMenuFactory);
        mScreenRecorderToolsMenu->clear();
        mScreenrecorderToolsMenuFactory->fillMenuFromGroupingNames(mScreenRecorderToolsMenu, { QStringLiteral("screenrecorder") });
    } );

    // the save menu
    mSaveAsAction = KStandardAction::saveAs(this, &KSMainWindow::saveAs, this);
    mSaveAction = KStandardAction::save(this, &KSMainWindow::save, this);
    mSaveMenu->addAction(mSaveAsAction);
    mSaveMenu->addAction(mSaveAction);
    setDefaultSaveAction();

    // message widget
    connect(mMessageWidget, &KMessageWidget::linkActivated, this, [](const QString &str) { QDesktopServices::openUrl(QUrl(str)); } );

    // layouts
    mDivider->setFrameShape(QFrame::HLine);
    mDivider->setLineWidth(2);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(mKSWidget);
    layout->addWidget(mMessageWidget);
    layout->addWidget(mDivider);
    layout->addWidget(mDialogButtonBox);
    mMessageWidget->hide();

    // populate our send-to actions
    mSendToButton->setMenu(mExportMenu);
    connect(mExportMenu, &ExportMenu::imageShared, this, &KSMainWindow::showImageSharedFeedback);

    // lock down the onClick mode depending on available shutter modes
    if (!mShutterModes.testFlag(Platform::ShutterMode::OnClick)) {
        mKSWidget->lockOnClickDisabled();
    } else if (!mShutterModes.testFlag(Platform::ShutterMode::Immediate)) {
        mKSWidget->lockOnClickEnabled();
    }
    resize(QSize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT).expandedTo(minimumSize()));

    // Allow Ctrl+Q to quit the app
    QAction *actionQuit = KStandardAction::quit(qApp, &QApplication::quit, this);
    actionQuit->setShortcut(QKeySequence::Quit);
    addAction(actionQuit);

    mHideMessageWidgetTimer = new QTimer(this);
    connect(mHideMessageWidgetTimer, &QTimer::timeout,
            mMessageWidget, &KMessageWidget::animatedHide);
    mHideMessageWidgetTimer->setInterval(10000);
    // done with the init
}

int KSMainWindow::windowWidth(const QPixmap &pixmap) const
{
    // Calculates what the width of the window should be for the captured image to perfectly fit
    // the area reserved for the image, with the height already set.

    const float pixmapAspectRatio = (float)pixmap.width() / pixmap.height();
    const int imageHeight = mKSWidget->height() - 2 * layout()->spacing();
    const int imageWidth = pixmapAspectRatio * imageHeight;

    int alignedWindowWidth = qMin(mKSWidget->imagePaddingWidth() + imageWidth, MAXIMUM_WINDOW_WIDTH);
    alignedWindowWidth += layout()->contentsMargins().left() + layout()->contentsMargins().right();
    alignedWindowWidth += 2; // margins is removing 1 - 1 pixel for some reason

    return alignedWindowWidth;
}

void KSMainWindow::setDefaultSaveAction()
{
    switch (SpectacleConfig::instance()->lastUsedSaveMode()) {
    case SaveMode::SaveAs:
    default:
        mSaveButton->setDefaultAction(mSaveAsAction);
        mSaveButton->setText(i18n("Save As..."));
        break;
    case SaveMode::Save:
        mSaveButton->setDefaultAction(mSaveAction);
        break;
    }
}

// overrides

void KSMainWindow::moveEvent(QMoveEvent *event)
{
    Q_UNUSED(event)

    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("spectaclerc"));
    KConfigGroup guiConfig(config, "GuiConfig");

    guiConfig.writeEntry("window-position", pos());
    guiConfig.sync();
}

// slots
void KSMainWindow::captureScreenshot(Spectacle::CaptureMode theCaptureMode, int theTimeout, bool theIncludePointer, bool theIncludeDecorations)
{
    if (theTimeout < 0) { // OnClick is checked (always the case on Wayland)
        hide();
        emit newScreenshotRequest(theCaptureMode, theTimeout, theIncludePointer, theIncludeDecorations);
        return;
    }

    showMinimized();
    mMessageWidget->hide();
    QTimer* timer = new QTimer;
    timer->setSingleShot(true);
    timer->setInterval(theTimeout);
    auto unityUpdate = [](const QVariantMap &properties) {
        QDBusMessage message = QDBusMessage::createSignal(QStringLiteral("/org/kde/Spectacle"),
            QStringLiteral("com.canonical.Unity.LauncherEntry"), QStringLiteral("Update"));
        message.setArguments({QGuiApplication::desktopFileName(), properties});
        QDBusConnection::sessionBus().send(message);
    };
    auto delayAnimation = new QVariantAnimation(timer);
    delayAnimation->setStartValue(0.0);
    delayAnimation->setEndValue(1.0);
    delayAnimation->setDuration(timer->interval());
    connect(delayAnimation, &QVariantAnimation::valueChanged, this, [=] {
        const double progress = delayAnimation->currentValue().toDouble();
        const double timeoutInSeconds = theTimeout / 1000.0;
        unityUpdate({ {QStringLiteral("progress"), progress} });
        setWindowTitle(i18ncp("@title:window", "%1 second", "%1 seconds",
            qMin(int(timeoutInSeconds), qCeil((1 - progress) * timeoutInSeconds))));
    });
    connect(timer, &QTimer::timeout, this, [=] {
        this->hide();
        timer->deleteLater();
        unityUpdate({ {QStringLiteral("progress-visible"), false} });
        emit newScreenshotRequest(theCaptureMode, 0, theIncludePointer, theIncludeDecorations);
    });

    unityUpdate({   {QStringLiteral("progress-visible"), true},
                    {QStringLiteral("progress"), 0 } });
    timer->start();
    delayAnimation->start();
}

void KSMainWindow::setScreenshotAndShow(const QPixmap &pixmap)
{
    mKSWidget->setScreenshotPixmap(pixmap);
    mExportMenu->imageUpdated();

    setWindowTitle(i18nc("@title:window Unsaved Screenshot", "Unsaved[*]"));
    setWindowModified(true);

    show();
    activateWindow();

    resize(QSize(windowWidth(pixmap), DEFAULT_WINDOW_HEIGHT));
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

void KSMainWindow::openScreenshotsFolder()
{
    // Highlight last screenshot in file manager if user saved at least once ever
    // (since last save and saveas file names are stored in spectaclerc), otherwise,
    //   if in save mode, open default save location from configure > save > location
    //   if in save as mode, open last save as files location
    // failsafe for either option is default save location from configure > save > location
    SpectacleConfig *cfgManager = SpectacleConfig::instance();
    ExportManager *exportManager = ExportManager::instance();
    QUrl location;

    switch(cfgManager->lastUsedSaveMode()) {
    case SaveMode::Save:
        location = cfgManager->lastSaveFile();
        if (!exportManager->isFileExists(location)) {
            location = cfgManager->defaultSaveLocation();
        }
        break;
    case SaveMode::SaveAs:
        location = cfgManager->lastSaveAsFile();  // already has a "/" at the end
        if (!exportManager->isFileExists(location)) {
            location = cfgManager->lastSaveAsLocation();
        }
        break;
    }

    KIO::highlightInFileManager({location});
}

void KSMainWindow::quit(const QuitBehavior quitBehavior)
{
    qApp->setQuitOnLastWindowClosed(false);
    hide();

    if (quitBehavior == QuitBehavior::QuitImmediately) {
        // Allow some time for clipboard content to transfer
        // TODO: Find better solution
        QTimer::singleShot(250, qApp, &QApplication::quit);
    }
    // TODO for else case:
    // Currently it is expected that you emit forceNotify, and finally quit
    // via a callback through KNotification::action1Activated. However, that
    // is not working quite right, see Bug #389694 which needs fixing.
}

void KSMainWindow::showInlineMessage(const QString& message, const KMessageWidget::MessageType messageType,
                                     const MessageDuration messageDuration, const QList<QAction*>& actions)
{
    const auto messageWidgetActions = mMessageWidget->actions();
    for (QAction* action: messageWidgetActions) {
        mMessageWidget->removeAction(action);
    }
    for (QAction* action : actions) {
        mMessageWidget->addAction(action);
    }
    mMessageWidget->setText(message);
    mMessageWidget->setMessageType(messageType);

    switch (messageType) {
    case KMessageWidget::Error:
        mMessageWidget->setIcon(QIcon::fromTheme(QStringLiteral("dialog-error")));
        break;
    case KMessageWidget::Warning:
        mMessageWidget->setIcon(QIcon::fromTheme(QStringLiteral("dialog-warning")));
        break;
    case KMessageWidget::Positive:
        mMessageWidget->setIcon(QIcon::fromTheme(QStringLiteral("dialog-ok-apply")));
        break;
    case KMessageWidget::Information:
        mMessageWidget->setIcon(QIcon::fromTheme(QStringLiteral("dialog-information")));
        break;
    }

    mHideMessageWidgetTimer->stop();
    mMessageWidget->animatedShow();
    if (messageDuration == MessageDuration::AutoHide) {
        mHideMessageWidgetTimer->start();
    }
}

void KSMainWindow::showImageSharedFeedback(bool error, const QString &message)
{
    if (error == 1) {
        // error == 1 means the user cancelled the sharing
        return;
    }

    if (error) {
        showInlineMessage(i18n("There was a problem sharing the image: %1", message),
                          KMessageWidget::Error);
    } else {
        if (message.isEmpty()) {
            showInlineMessage(i18n("Image shared"), KMessageWidget::Positive);
        } else {
            showInlineMessage(i18n("The shared image link (<a href=\"%1\">%1</a>) has been copied to the clipboard.", message),
                              KMessageWidget::Positive, MessageDuration::Persistent);
            QApplication::clipboard()->setText(message);
        }
    }
}

void KSMainWindow::sendToClipboard()
{
    bool notify = false;
    ExportManager::instance()->doCopyToClipboard(notify);

    SpectacleConfig::instance()->quitAfterSaveOrCopyChecked()
            ? quit()
            : showInlineMessage(i18n("The screenshot has been copied to the clipboard."),
                                KMessageWidget::Information);
}

void KSMainWindow::showPreferencesDialog()
{
    SettingsDialog prefDialog(this);
    prefDialog.exec();
}

void KSMainWindow::imageSaved(const QUrl &location)
{
    setWindowTitle(location.fileName());
    setWindowModified(false);
    QAction* openContaining = new QAction(QIcon::fromTheme(QStringLiteral("document-open-folder")), i18n("Open Containing Folder"), mMessageWidget);
    connect(openContaining, &QAction::triggered, [=] { KIO::highlightInFileManager({location});});
    showInlineMessage(i18n("The screenshot was saved as <a href=\"%1\">%2</a>",
                           location.toString(), location.fileName()), KMessageWidget::Positive,
                           MessageDuration::AutoHide, {openContaining});
}

void KSMainWindow::save()
{
    SpectacleConfig::instance()->setLastUsedSaveMode(SaveMode::Save);
    setDefaultSaveAction();

    const bool quitChecked = SpectacleConfig::instance()->quitAfterSaveOrCopyChecked();
    ExportManager::instance()->doSave(QUrl(), /* notify */ quitChecked);
    if (quitChecked) {
        quit(QuitBehavior::QuitExternally);
    }
}

void KSMainWindow::saveAs()
{
    SpectacleConfig::instance()->setLastUsedSaveMode(SaveMode::SaveAs);
    setDefaultSaveAction();

    const bool quitChecked = SpectacleConfig::instance()->quitAfterSaveOrCopyChecked();
    if (ExportManager::instance()->doSaveAs(this, /* notify */ quitChecked) && quitChecked) {
        quit(QuitBehavior::QuitExternally);
    }
}
