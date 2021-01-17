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

#include "settings.h"
#include "SettingsDialog/SettingsDialog.h"
#include "SettingsDialog/GeneralOptionsPage.h"
#include "SettingsDialog/SaveOptionsPage.h"
#include "SettingsDialog/ShortcutsOptionsPage.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QKeyEvent>
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
#include <KIO/OpenUrlJob>
#include <KIO/JobUiDelegate>
#include <KLocalizedString>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KStandardAction>
#include <KWindowSystem>

static const int DEFAULT_WINDOW_HEIGHT = 420;
static const int DEFAULT_WINDOW_WIDTH = 840;
static const int MAXIMUM_WINDOW_WIDTH = 1000;

KSMainWindow::KSMainWindow(Platform::GrabModes theGrabModes, Platform::ShutterModes theShutterModes, QWidget *parent) :
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
#ifdef KIMAGEANNOTATOR_FOUND
    ,mAnnotateButton(new QToolButton(this))
    ,mAnnotatorActive(false)
#endif
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

    // change window title on save and on autosave

    connect(ExportManager::instance(), &ExportManager::imageSaved, this, &KSMainWindow::imageSaved);
    connect(ExportManager::instance(), &ExportManager::imageCopied, this, &KSMainWindow::imageCopied);
    connect(ExportManager::instance(), &ExportManager::imageSavedAndCopied, this, &KSMainWindow::imageSavedAndCopied);

    // the KSGWidget

    connect(mKSWidget, &KSWidget::newScreenshotRequest, this, &KSMainWindow::captureScreenshot);
    connect(mKSWidget, &KSWidget::dragInitiated, this, &KSMainWindow::dragAndDropRequest);

    // the Button Bar

    mDialogButtonBox->setStandardButtons(QDialogButtonBox::Help);
    mDialogButtonBox->button(QDialogButtonBox::Help)->setAutoDefault(false);

    mConfigureButton->setDefaultAction(KStandardAction::preferences(this, SLOT(showPreferencesDialog()), this));
    mConfigureButton->setText(i18n("Configure..."));
    mConfigureButton->setToolTip(i18n("Change Spectacle's settings."));
    mConfigureButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mDialogButtonBox->addButton(mConfigureButton, QDialogButtonBox::ResetRole);

#ifdef KIMAGEANNOTATOR_FOUND
    mAnnotateButton->setText(i18n("Annotate"));
    mAnnotateButton->setToolTip(i18n("Add annotation to the screenshot"));
    mAnnotateButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mAnnotateButton->setIcon(QIcon::fromTheme(QStringLiteral("document-edit")));
    connect(mAnnotateButton, &QToolButton::clicked, this, [this] {

        if (mAnnotatorActive) {
            mKSWidget->hideAnnotator();
            mAnnotateButton->setText(i18n("Annotate"));
        } else {
            mKSWidget->showAnnotator();
            mAnnotateButton->setText(i18n("Annotation done"));
        }

        mToolsButton->setEnabled(mAnnotatorActive);
        mSendToButton->setEnabled(mAnnotatorActive);
        mClipboardButton->setEnabled(mAnnotatorActive);
        mSaveButton->setEnabled(mAnnotatorActive);

        mAnnotatorActive = !mAnnotatorActive;
    });

    mDialogButtonBox->addButton(mAnnotateButton, QDialogButtonBox::ActionRole);
#endif

    KGuiItem::assign(mToolsButton, KGuiItem(i18n("Tools")));
    mToolsButton->setIcon(QIcon::fromTheme(QStringLiteral("tools"),
                                           QIcon::fromTheme(QStringLiteral("application-menu"))));
    mToolsButton->setAutoDefault(false);
    mDialogButtonBox->addButton(mToolsButton, QDialogButtonBox::ActionRole);
    mToolsButton->setMenu(mToolsMenu);

    KGuiItem::assign(mSendToButton, KGuiItem(i18n("Export")));
    mSendToButton->setIcon(QIcon::fromTheme(QStringLiteral("document-share")));
    mSendToButton->setAutoDefault(false);
    mDialogButtonBox->addButton(mSendToButton, QDialogButtonBox::ActionRole);

    mClipboardButton->setDefaultAction(KStandardAction::copy(this, &KSMainWindow::copy, this));
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
                          i18n("Open Default Screenshots Folder"),
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

    // message: open containing folder
    mOpenContaining = new QAction(QIcon::fromTheme(QStringLiteral("document-open-folder")), i18n("Open Containing Folder"), mMessageWidget);
    connect(mOpenContaining, &QAction::triggered, [=] { KIO::highlightInFileManager({Settings::lastSaveLocation()});});

    mHideMessageWidgetTimer = new QTimer(this);
//     connect(mHideMessageWidgetTimer, &QTimer::timeout,
//             mMessageWidget, &KMessageWidget::animatedHide);
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
    switch (Settings::lastUsedSaveMode()) {
    case Settings::SaveAs:
        mSaveButton->setDefaultAction(mSaveAsAction);
        mSaveButton->setText(i18n("Save As..."));
        break;
    case Settings::Save:
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
        mKSWidget->setProgress(progress);
        unityUpdate({ {QStringLiteral("progress"), progress} });
        setWindowTitle(i18ncp("@title:window", "%1 second", "%1 seconds",
            qMin(int(timeoutInSeconds), qCeil((1 - progress) * timeoutInSeconds))));
    });
    connect(timer, &QTimer::timeout, this, [=] {
        this->hide();
        timer->deleteLater();
        mKSWidget->setProgress(0);
        unityUpdate({ {QStringLiteral("progress-visible"), false} });
        emit newScreenshotRequest(theCaptureMode, 0, theIncludePointer, theIncludeDecorations);
    });

    connect(mKSWidget, &KSWidget::screenshotCanceled, timer, [=] {
        timer->stop();
        timer->deleteLater();
        restoreWindowTitle();
        unityUpdate({ {QStringLiteral("progress-visible"), false} });
    });


    unityUpdate({   {QStringLiteral("progress-visible"), true},
                    {QStringLiteral("progress"), 0 } });
    timer->start();
    delayAnimation->start();
}

void KSMainWindow::setScreenshotAndShow(const QPixmap &pixmap)
{
    if (!pixmap.isNull()) {
        mKSWidget->setScreenshotPixmap(pixmap);
        mExportMenu->imageUpdated();
        setWindowTitle(i18nc("@title:window Unsaved Screenshot", "Unsaved[*]"));
        setWindowModified(true);
    } else {
        restoreWindowTitle();
    }
    mKSWidget->setButtonState(KSWidget::State::TakeNewScreenshot);
    show();
    activateWindow();
    /* NOTE windowWidth only produces the right result if it is called after the window is visible.
     * Because of this the call is not moved into the if above */
    if(!pixmap.isNull()) {
        resize(QSize(windowWidth(pixmap), DEFAULT_WINDOW_HEIGHT));
    }
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
    auto job = new KIO::OpenUrlJob(Settings::defaultSaveLocation());
    job->setUiDelegate(new KIO::JobUiDelegate(KIO::JobUiDelegate::AutoHandlingEnabled, this));
    job->start();
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

void KSMainWindow::copy()
{
    const bool quitChecked = Settings::quitAfterSaveCopyExport();
    ExportManager::instance()->doCopyToClipboard();
    if (quitChecked) {
        quit(QuitBehavior::QuitExternally);
    }
}

void KSMainWindow::imageCopied()
{
    showInlineMessage(i18n("The screenshot has been copied to the clipboard."),
                      KMessageWidget::Information);
}

void KSMainWindow::screenshotFailed()
{
    showInlineMessage(i18n("Could not take a screenshot. Please report this bug here: <a href=\"https://bugs.kde.org/enter_bug.cgi?product=Spectacle\">create a spectacle bug</a>"),
                      KMessageWidget::Warning);
}

void KSMainWindow::showPreferencesDialog()
{
    if (KConfigDialog::showDialog(QStringLiteral("settings"))) {
        return;
    }
    (new SettingsDialog(this))->show();
}

void KSMainWindow::imageSaved(const QUrl &location)
{
    setWindowTitle(location.fileName());
    setWindowModified(false);
    showInlineMessage(i18n("The screenshot was saved as <a href=\"%1\">%2</a>",
                           location.toString(), location.fileName()), KMessageWidget::Positive,
                           MessageDuration::AutoHide, {mOpenContaining});
}

void KSMainWindow::imageSavedAndCopied(const QUrl &location)
{
    setWindowTitle(location.fileName());
    setWindowModified(false);
    showInlineMessage(i18n("The screenshot was copied to the clipboard and saved as <a href=\"%1\">%2</a>",
                           location.toString(), location.fileName()), KMessageWidget::Positive,
                           MessageDuration::AutoHide, {mOpenContaining});
}

void KSMainWindow::save()
{
    Settings::setLastUsedSaveMode(Settings::Save);
    setDefaultSaveAction();

    const bool quitChecked = Settings::quitAfterSaveCopyExport();
    ExportManager::instance()->doSave(QUrl(), /* notify */ quitChecked);
    if (quitChecked) {
        quit(QuitBehavior::QuitExternally);
    }
}

void KSMainWindow::saveAs()
{
    Settings::setLastUsedSaveMode(Settings::SaveAs);
    setDefaultSaveAction();

    const bool quitChecked = Settings::quitAfterSaveCopyExport();
    if (ExportManager::instance()->doSaveAs(this, /* notify */ quitChecked) && quitChecked) {
        quit(QuitBehavior::QuitExternally);
    }
}

void KSMainWindow::restoreWindowTitle()
{
    if (isWindowModified()) {
        setWindowTitle(i18nc("@title:window Unsaved Screenshot", "Unsaved[*]"));
    } else {
        setWindowTitle(Settings::lastSaveLocation().fileName());
    }
}

/* This event handler enables all Buttons to be activated with Enter. Normally only QPushButton can
 * be activated with Enter but we also use QToolButtons so we handle the event ourselves */
void KSMainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return) {
        QWidget *fw = focusWidget();
        auto pb = qobject_cast<QPushButton*>(fw);
        if (pb) {
            pb->animateClick();
            return;
        }
        auto tb = qobject_cast<QToolButton*>(fw);
        if (tb) {
            tb->animateClick();
            return;
        }
    }
    QDialog::keyPressEvent(event);
}
