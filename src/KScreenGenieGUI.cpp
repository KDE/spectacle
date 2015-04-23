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

#include "KScreenGenieGUI.h"

KScreenGenieGUI::KScreenGenieGUI(bool onClickAvailable, QWidget *parent) :
    QWidget(parent),
    mQuickWidget(nullptr),
    mDialogButtonBox(nullptr),
    mSendToButton(nullptr),
    mPrintButton(nullptr),
    mSendToMenu(new KSGSendToMenu),
    mKQmlObject(new KDeclarative::QmlObject),
    mScreenshotImageProvider(new KSGImageProvider),
    mOnClickAvailable(onClickAvailable)
{
    QMetaObject::invokeMethod(this, "init", Qt::QueuedConnection);
}

KScreenGenieGUI::~KScreenGenieGUI()
{
    if (mQuickWidget) {
        delete mQuickWidget;
        delete mKQmlObject;
    }
}

// GUI init

void KScreenGenieGUI::init()
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kscreengenierc");
    KConfigGroup guiConfig(config, "GuiConfig");

    // window properties

    setWindowTitle(i18nc("Untitled Screenshot", "Untitled"));
    setFixedSize(800, 370);

    QPoint location = guiConfig.readEntry("window-position", QPoint(50, 50));
    move(location);

    // the QtQuick widget

    mKQmlObject->engine()->addImageProvider(QLatin1String("screenshot"), mScreenshotImageProvider);
    mQuickWidget = new QQuickWidget(mKQmlObject->engine(), this);

    mQuickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    mQuickWidget->resize(mQuickWidget->width(), 300);
    mQuickWidget->setClearColor(QWidget::palette().color(QWidget::backgroundRole()));
    mQuickWidget->setSource(QUrl("qrc:///MainForm.qml"));

    // connect the qml signals

    QQuickItem *rootItem = mQuickWidget->rootObject();
    connect(rootItem, SIGNAL(newScreenshotRequest(QString,double,bool,bool)), this, SLOT(captureScreenshot(QString,double,bool,bool)));
    connect(rootItem, SIGNAL(saveCheckboxStates(bool,bool,bool)), this, SLOT(saveCheckboxStatesConfig(bool,bool,bool)));
    connect(rootItem, SIGNAL(saveCaptureMode(int)), this, SLOT(saveCaptureModeConfig(int)));
    connect(rootItem, SIGNAL(startDragAndDrop()), this, SIGNAL(dragAndDropRequest()));
    connect(rootItem, SIGNAL(startDragAndDrop()), this, SLOT(ungrabMouseWorkaround()));

    // the Button Bar

    mDialogButtonBox = new QDialogButtonBox(this);
    mDialogButtonBox->setStandardButtons(QDialogButtonBox::Help | QDialogButtonBox::Discard | QDialogButtonBox::Apply | QDialogButtonBox::Save);

    mSendToButton = new QPushButton;
    KGuiItem::assign(mSendToButton, KGuiItem(i18n("Send To...")));
    mDialogButtonBox->addButton(mSendToButton, QDialogButtonBox::ActionRole);

    mPrintButton = new QPushButton;
    KGuiItem::assign(mPrintButton, KStandardGuiItem::print());
    connect(mPrintButton, &QPushButton::clicked, this, &KScreenGenieGUI::showPrintDialog);
    mDialogButtonBox->addButton(mPrintButton, QDialogButtonBox::ActionRole);

    connect(mDialogButtonBox->button(QDialogButtonBox::Discard), &QPushButton::clicked, qApp, &QApplication::quit);
    connect(mDialogButtonBox->button(QDialogButtonBox::Save), &QPushButton::clicked, this, &KScreenGenieGUI::saveAsClicked);
    KGuiItem::assign(mDialogButtonBox->button(QDialogButtonBox::Save), KStandardGuiItem::saveAs());

    connect(mDialogButtonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &KScreenGenieGUI::saveAndExit);
    KGuiItem::assign(mDialogButtonBox->button(QDialogButtonBox::Apply), KStandardGuiItem::save());
    mDialogButtonBox->button(QDialogButtonBox::Apply)->setText(i18n("Save && Exit"));
    mDialogButtonBox->button(QDialogButtonBox::Apply)->setToolTip(i18n("Quicksave screenshot in your Pictures directory and exit"));

    KHelpMenu *helpMenu = new KHelpMenu(this, KAboutData::applicationData(), true);
    mDialogButtonBox->button(QDialogButtonBox::Help)->setMenu(helpMenu->menu());

    // layouts

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mQuickWidget);
    layout->addWidget(mDialogButtonBox);
    setLayout(layout);
    mQuickWidget->setFocus();

    // populate our send-to actions

    connect(mSendToMenu, &KSGSendToMenu::sendToServiceRequest, this, &KScreenGenieGUI::sendToKServiceRequest);
    connect(mSendToMenu, &KSGSendToMenu::sendToClipboardRequest, this, &KScreenGenieGUI::sendToClipboardRequest);
    connect(mSendToMenu, &KSGSendToMenu::sendToOpenWithRequest, this, &KScreenGenieGUI::sendToOpenWithRequest);

    mSendToButton->setMenu(mSendToMenu->menu());

    // read in the checkbox states and capture mode index

    bool includePointer = guiConfig.readEntry("includePointer", true);
    bool includeDecorations = guiConfig.readEntry("includeDecorations", true);
    bool waitCaptureOnClick = guiConfig.readEntry("waitCaptureOnClick", false);
    QMetaObject::invokeMethod(rootItem, "loadCheckboxStates",
                              Q_ARG(QVariant, includePointer),
                              Q_ARG(QVariant, includeDecorations),
                              Q_ARG(QVariant, waitCaptureOnClick)
    );

    int captureModeIndex = guiConfig.readEntry("captureModeIndex", 0);
    QMetaObject::invokeMethod(rootItem, "loadCaptureMode", Q_ARG(QVariant, captureModeIndex));

    // disable onClick mode if not available on the platform

    if (!mOnClickAvailable) {
        QMetaObject::invokeMethod(rootItem, "disableOnClick");
    }

    // done with the init
}

// overrides

void KScreenGenieGUI::moveEvent(QMoveEvent *event)
{
    Q_UNUSED(event);

    KSharedConfigPtr config = KSharedConfig::openConfig("kscreengenierc");
    KConfigGroup guiConfig(config, "GuiConfig");

    guiConfig.writeEntry("window-position", pos());
    guiConfig.sync();
}

// slots

void KScreenGenieGUI::captureScreenshot(QString captureMode, double captureDelay, bool includePointer, bool includeDecorations)
{
    hide();

    ImageGrabber::GrabMode mode = ImageGrabber::InvalidChoice;
    int msec = captureDelay * 1000;;

    if (captureMode == QStringLiteral("fullScreen")) {
        mode = ImageGrabber::FullScreen;
    } else if (captureMode == QStringLiteral("currentScreen")) {
        mode = ImageGrabber::CurrentScreen;
    } else if (captureMode == QStringLiteral("activeWindow")) {
        mode = ImageGrabber::ActiveWindow;
    } else if (captureMode == QStringLiteral("rectangularRegion")) {
        mode = ImageGrabber::RectangularRegion;
    } else {
        qWarning() << "Capture called with invalid mode";
        show();
        return;
    }

    emit newScreenshotRequest(mode, msec, includePointer, includeDecorations);
}

void KScreenGenieGUI::setScreenshotAndShow(const QPixmap &pixmap)
{
    mScreenshotImageProvider->setPixmap(pixmap);

    QQuickItem *rootItem = mQuickWidget->rootObject();
    QMetaObject::invokeMethod(rootItem, "reloadScreenshot");

    show();
    if (mSendToMenu->menu()->isEmpty()) {
        mSendToMenu->populateMenu();
    }
}

void KScreenGenieGUI::ungrabMouseWorkaround()
{
    QQuickItem *dragMouseArea = mQuickWidget->rootObject()->findChild<QQuickItem *>("screenshotDragMouseArea");
    QMouseEvent releaseEvent(QMouseEvent::MouseButtonRelease, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    qApp->sendEvent(mQuickWidget, &releaseEvent);
    dragMouseArea->ungrabMouse();
}

void KScreenGenieGUI::showPrintDialog()
{
    QPrinter *printer = new QPrinter(QPrinter::HighResolution);
    QPrintDialog printDialog(printer, this);
    if (printDialog.exec() == QDialog::Accepted) {
        emit printRequest(printer);
        return;
    }
    delete printer;
}

void KScreenGenieGUI::saveCheckboxStatesConfig(bool includePointer, bool includeDecorations, bool waitCaptureOnClick)
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kscreengenierc");
    KConfigGroup guiConfig(config, "GuiConfig");

    guiConfig.writeEntry("includePointer", includePointer);
    guiConfig.writeEntry("includeDecorations", includeDecorations);
    guiConfig.writeEntry("waitCaptureOnClick", waitCaptureOnClick);
    guiConfig.sync();
}

void KScreenGenieGUI::saveCaptureModeConfig(int modeIndex)
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kscreengenierc");
    KConfigGroup guiConfig(config, "GuiConfig");

    guiConfig.writeEntry("captureModeIndex", modeIndex);
    guiConfig.sync();
}
