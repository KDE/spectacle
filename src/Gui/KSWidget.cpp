/*
 *  Copyright 2019 David Redondo <kde@david-redondo.de>
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

#include "KSWidget.h"
#include "spectacle_gui_debug.h"

#include "KSImageWidget.h"
#include "settings.h"
#include "SmartSpinBox.h"
#include "ProgressButton.h"
#include "ExportManager.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
#include <QShortcut>
#include <QStackedLayout>

#ifdef KIMAGEANNOTATOR_FOUND
#include <kImageAnnotator/KImageAnnotator.h>
#endif

#include <KConfigDialogManager>
#include <KLocalizedString>

KSWidget::KSWidget(Platform::GrabModes theGrabModes, QWidget *parent)
    : QWidget(parent)
{
    mStack = new QStackedLayout(this);

    // we'll init the widget that holds the image first
    mImageWidget = new KSImageWidget(this);
    mImageWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(mImageWidget, &KSImageWidget::dragInitiated, this, &KSWidget::dragInitiated);

#ifdef KIMAGEANNOTATOR_FOUND
    mAnnotator = new kImageAnnotator::KImageAnnotator();
    mAnnotator->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mStack->addWidget(mAnnotator);
#endif

    // the capture mode options first
    mCaptureModeLabel = new QLabel(i18n("<b>Capture Mode</b>"), this);
    mCaptureArea = new QComboBox(this);
    QString lFullScreenLabel = QApplication::screens().count() == 1
            ? i18n("Full Screen")
            : i18n("Full Screen (All Monitors)");

    if (theGrabModes.testFlag(Platform::GrabMode::AllScreens)) {
        mCaptureArea->insertItem(0, lFullScreenLabel, Spectacle::CaptureMode::AllScreens);
        mCaptureArea->insertItem(1, i18n("Rectangular Region"), Spectacle::CaptureMode::RectangularRegion);
    }
    if (theGrabModes.testFlag(Platform::GrabMode::CurrentScreen)) {
        mCaptureArea->insertItem(2, i18n("Current Screen"), Spectacle::CaptureMode::CurrentScreen);
    }
    if (theGrabModes.testFlag(Platform::GrabMode::ActiveWindow)) {
        mCaptureArea->insertItem(3, i18n("Active Window"), Spectacle::CaptureMode::ActiveWindow);
    }
    if (theGrabModes.testFlag(Platform::GrabMode::WindowUnderCursor)) {
        mCaptureArea->insertItem(4, i18n("Window Under Cursor"), Spectacle::CaptureMode::WindowUnderCursor);
    }
    if (theGrabModes.testFlag(Platform::GrabMode::TransientWithParent)) {
        mTransientWithParentAvailable = true;
    }
    mCaptureArea->setMinimumWidth(240);
    mCaptureArea->setObjectName(QStringLiteral("kcfg_captureMode"));
    mCaptureArea->setProperty("kcfg_property", QByteArray("currentData"));
    connect(mCaptureArea, qOverload<int>(&QComboBox::currentIndexChanged), this, &KSWidget::captureModeChanged);

    mDelayMsec = new SmartSpinBox(this);
    mDelayMsec->setDecimals(1);
    mDelayMsec->setSingleStep(1.0);
    mDelayMsec->setMinimum(0.0);
    mDelayMsec->setMaximum(999.9);
    mDelayMsec->setSpecialValueText(i18n("No Delay"));
    mDelayMsec->setMinimumWidth(160);
    mDelayMsec->setObjectName(QStringLiteral("kcfg_captureDelay"));

    mCaptureOnClick = new QCheckBox(i18n("On Click"), this);
    mCaptureOnClick->setToolTip(i18n("Wait for a mouse click before capturing the screenshot image"));
    connect(mCaptureOnClick, &QCheckBox::stateChanged, this, &KSWidget::onClickStateChanged);
    mCaptureOnClick->setObjectName(QStringLiteral("kcfg_onClickChecked"));

    mDelayLayout = new QHBoxLayout;
    mDelayLayout->addWidget(mDelayMsec);
    mDelayLayout->addWidget(mCaptureOnClick);

    mCaptureModeForm = new QFormLayout;
    mCaptureModeForm->addRow(i18n("Area:"), mCaptureArea);
    mCaptureModeForm->addRow(i18n("Delay:"), mDelayLayout);
    mCaptureModeForm->setContentsMargins(24, 0, 0, 0);

    // options (mouse pointer, window decorations, quit after saving or copying)
    mContentOptionsLabel = new QLabel(this);
    mContentOptionsLabel->setText(i18n("<b>Options</b>"));

    mMousePointer = new QCheckBox(i18n("Include mouse pointer"), this);
    mMousePointer->setToolTip(i18n("Show the mouse cursor in the screenshot image"));
    mMousePointer->setObjectName(QStringLiteral("kcfg_includePointer"));

    mWindowDecorations = new QCheckBox(i18n("Include window titlebar and borders"), this);
    mWindowDecorations->setToolTip(i18n("Show the window title bar, the minimize/maximize/close buttons, and the window border"));
    mWindowDecorations->setEnabled(false);
    mWindowDecorations->setObjectName(QStringLiteral("kcfg_includeDecorations"));

    mCaptureTransientOnly = new QCheckBox(i18n("Capture the current pop-up only"), this);
    mCaptureTransientOnly->setToolTip(i18n("Capture only the current pop-up window (like a menu, tooltip etc).\n"
                                           "If disabled, the pop-up is captured along with the parent window"));
    mCaptureTransientOnly->setEnabled(false);
    mCaptureTransientOnly->setObjectName(QStringLiteral("kcfg_transientOnly"));

    mQuitAfterSaveOrCopy = new QCheckBox(i18n("Quit after manual Save or Copy"), this);
    mQuitAfterSaveOrCopy->setToolTip(i18n("Quit Spectacle after manually saving or copying the image"));
    mQuitAfterSaveOrCopy->setObjectName(QStringLiteral("kcfg_quitAfterSaveCopyExport"));

    mContentOptionsForm = new QVBoxLayout;
    mContentOptionsForm->addWidget(mMousePointer);
    mContentOptionsForm->addWidget(mWindowDecorations);
    mContentOptionsForm->addWidget(mCaptureTransientOnly);
    mContentOptionsForm->addWidget(mQuitAfterSaveOrCopy);
    mContentOptionsForm->setContentsMargins(24, 0, 0, 0);

    mTakeNewScreenshotAction = new QAction(QIcon::fromTheme(QStringLiteral("spectacle")), i18n("Take a New Screenshot"), this);
    mTakeNewScreenshotAction->setShortcut(QKeySequence::New);
    connect(mTakeNewScreenshotAction, &QAction::triggered, this, &KSWidget::newScreenshotClicked);

    mCancelAction = new QAction(QIcon::fromTheme(QStringLiteral("dialog-cancel")), i18n("Cancel"), this);
    mCancelAction->setShortcut(QKeySequence::Cancel);
    connect(mCancelAction, &QAction::triggered, this, [this] {
        emit screenshotCanceled();
        setButtonState(State::TakeNewScreenshot);
    });

    // the take a new screenshot button
    mTakeScreenshotButton = new ProgressButton(this);
    mTakeScreenshotButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    mTakeScreenshotButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setButtonState(State::TakeNewScreenshot);
    mTakeScreenshotButton->setFocus();

    // finally, finish up the layouts
    mRightLayout = new QVBoxLayout;
    mRightLayout->addStretch(1);
    mRightLayout->addWidget(mCaptureModeLabel);
    mRightLayout->addLayout(mCaptureModeForm);
    mRightLayout->addStretch(1);
    mRightLayout->addWidget(mContentOptionsLabel);
    mRightLayout->addLayout(mContentOptionsForm);
    mRightLayout->addStretch(10);
    mRightLayout->addWidget(mTakeScreenshotButton, 1, Qt::AlignHCenter);
    mRightLayout->setContentsMargins(10, 0, 0, 10);

    mMainLayout = new QGridLayout();

    mMainLayout->addWidget(mImageWidget, 0, 0, 1, 1);
    mMainLayout->addLayout(mRightLayout, 0, 1, 1, 1);
    mMainLayout->setColumnMinimumWidth(0, 320);
    mMainLayout->setColumnMinimumWidth(1, 320);

    int index = mCaptureArea->findData(Settings::captureMode());
    mCaptureArea->setCurrentIndex(index >= 0 ? index : 0);
    auto mConfigManager = new KConfigDialogManager(this, Settings::self());
    connect(mConfigManager, &KConfigDialogManager::widgetModified, mConfigManager, &KConfigDialogManager::updateSettings);

    placeHolder = new QWidget();
    placeHolder->setLayout(mMainLayout);

    mStack->addWidget(placeHolder);

#ifdef KIMAGEANNOTATOR_FOUND
    mStack->addWidget(mAnnotator);
#endif
}

int KSWidget::imagePaddingWidth() const
{
    int lRightLayoutLeft  = 0;
    int lRightLayoutRight = 0;
    int lMainLayoutRight  = 0;

    mRightLayout->getContentsMargins(&lRightLayoutLeft, nullptr, &lRightLayoutRight, nullptr);
    mMainLayout->getContentsMargins(nullptr, nullptr, &lMainLayoutRight, nullptr);

    int lPaddingWidth = (lRightLayoutLeft + lRightLayoutRight + lMainLayoutRight);
    lPaddingWidth += mRightLayout->contentsRect().width();
    lPaddingWidth += 2 * SpectacleImage::SHADOW_RADIUS; // image drop shadow

    return lPaddingWidth;
}

// public slots

void KSWidget::setScreenshotPixmap(const QPixmap &thePixmap)
{
    mImageWidget->setScreenshot(thePixmap);
}

void KSWidget::lockOnClickEnabled()
{
    mCaptureOnClick->setCheckState(Qt::Checked);
    mCaptureOnClick->setEnabled(false);
    mDelayMsec->setEnabled(false);
}

void KSWidget::lockOnClickDisabled()
{
    mCaptureOnClick->setCheckState(Qt::Unchecked);
    mCaptureOnClick->setEnabled(false);
    mDelayMsec->setEnabled(true);
}

// private slots

void KSWidget::newScreenshotClicked()
{
    int lDelay = mCaptureOnClick->isChecked() ? -1 : (mDelayMsec->value() * 1000);
    auto lMode = static_cast<Spectacle::CaptureMode>(mCaptureArea->currentData().toInt());
    if (mTransientWithParentAvailable &&
        lMode == Spectacle::CaptureMode::WindowUnderCursor &&
        !(mCaptureTransientOnly->isChecked())) {
        lMode = Spectacle::CaptureMode::TransientWithParent;
    }
    setButtonState(State::Cancel);
    emit newScreenshotRequest(lMode, lDelay, mMousePointer->isChecked(), mWindowDecorations->isChecked());
}

void KSWidget::onClickStateChanged(int theState)
{
    if (theState == Qt::Checked) {
        mDelayMsec->setEnabled(false);
    } else if (theState == Qt::Unchecked) {
        mDelayMsec->setEnabled(true);
    }
}

void KSWidget::captureModeChanged(int theIndex)
{
    Spectacle::CaptureMode captureMode = static_cast<Spectacle::CaptureMode>(mCaptureArea->itemData(theIndex).toInt());
    switch(captureMode) {
    case Spectacle::CaptureMode::WindowUnderCursor:
        mWindowDecorations->setEnabled(true);
        if (mTransientWithParentAvailable) {
            mCaptureTransientOnly->setEnabled(true);
        } else {
            mCaptureTransientOnly->setEnabled(false);
        }
        break;
    case Spectacle::CaptureMode::ActiveWindow:
        mWindowDecorations->setEnabled(true);
        mCaptureTransientOnly->setEnabled(false);
        break;
    case Spectacle::CaptureMode::AllScreens:
    case Spectacle::CaptureMode::CurrentScreen:
    case Spectacle::CaptureMode::RectangularRegion:
        mWindowDecorations->setEnabled(false);
        mCaptureTransientOnly->setEnabled(false);
        break;
    case Spectacle::CaptureMode::TransientWithParent:
    case Spectacle::CaptureMode::InvalidChoice:
    default:
        qCWarning(SPECTACLE_GUI_LOG) << "Skipping invalid or unreachable enum value";
        break;
    }
}

void KSWidget::setButtonState(State state)
{
    switch (state) {
    case State::TakeNewScreenshot:
        mTakeScreenshotButton->removeAction(mCancelAction);
        mTakeScreenshotButton->setDefaultAction(mTakeNewScreenshotAction);
        mTakeScreenshotButton->setProgress(0);
        break;
    case State::Cancel:
        mTakeScreenshotButton->removeAction(mTakeNewScreenshotAction);
        mTakeScreenshotButton->setDefaultAction(mCancelAction);
        break;
    }
}

void KSWidget::setProgress(double progress)
{
    mTakeScreenshotButton->setProgress(progress);
}

#ifdef KIMAGEANNOTATOR_FOUND
void KSWidget::showAnnotator()
{
    mStack->setCurrentIndex(1);
    mAnnotator->loadImage(ExportManager::instance()->pixmap());
}

void KSWidget::hideAnnotator()
{
    mStack->setCurrentIndex(0);
    QImage image = mAnnotator->image();
    QPixmap px = QPixmap::fromImage(image);
    setScreenshotPixmap(px);
    ExportManager::instance()->setPixmap(px);
}
#endif
