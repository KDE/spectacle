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

#include "KSWidget.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QTimer>

#include <KLocalizedString>

#include "KSImageWidget.h"
#include "SmartSpinBox.h"

#include "SpectacleConfig.h"

KSWidget::KSWidget(QWidget *parent) :
    QWidget(parent)
{
    // get a handle to the configuration manager

    SpectacleConfig *configManager = SpectacleConfig::instance();

    // we'll init the widget that holds the image first

    mImageWidget = new KSImageWidget(this);
    mImageWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(mImageWidget, &KSImageWidget::dragInitiated, this, &KSWidget::dragInitiated);

    // the capture mode options first

    mCaptureModeLabel = new QLabel(i18n("<b>Capture Mode</b>"), this);

    mCaptureArea = new QComboBox(this);
    mCaptureArea->insertItem(1, i18n("Full Screen (All Monitors)"), ImageGrabber::FullScreen);
    mCaptureArea->insertItem(2, i18n("Current Screen"), ImageGrabber::CurrentScreen);
    mCaptureArea->insertItem(3, i18n("Active Window"), ImageGrabber::ActiveWindow);
    mCaptureArea->insertItem(4, i18n("Window Under Cursor"), ImageGrabber::WindowUnderCursor);
    mCaptureArea->insertItem(5, i18n("Rectangular Region"), ImageGrabber::RectangularRegion);
    mCaptureArea->setMinimumWidth(240);
    connect(mCaptureArea, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &KSWidget::captureModeChanged);

    mDelayMsec = new SmartSpinBox(this);
    mDelayMsec->setDecimals(1);
    mDelayMsec->setSingleStep(1.0);
    mDelayMsec->setMinimum(0.0);
    mDelayMsec->setMaximum(999.9);
    mDelayMsec->setSpecialValueText(i18n("No Delay"));
    mDelayMsec->setMinimumWidth(160);
    connect(mDelayMsec, static_cast<void (SmartSpinBox::*)(qreal)>(&SmartSpinBox::valueChanged),
            configManager, &SpectacleConfig::setCaptureDelay);

    mCaptureOnClick = new QCheckBox(i18n("On Click"), this);
    mCaptureOnClick->setToolTip(i18n("Wait for a mouse click before capturing the screenshot image"));
    connect(mCaptureOnClick, &QCheckBox::stateChanged, this, &KSWidget::onClickStateChanged);
    connect(mCaptureOnClick, &QCheckBox::clicked, configManager, &SpectacleConfig::setOnClickChecked);

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
    connect(mMousePointer, &QCheckBox::clicked, configManager, &SpectacleConfig::setIncludePointerChecked);

    mWindowDecorations = new QCheckBox(i18n("Include window titlebar and borders"), this);
    mWindowDecorations->setToolTip(i18n("Show the window title bar, the minimize/maximize/close buttons, and the window border"));
    mWindowDecorations->setEnabled(false);
    connect(mWindowDecorations, &QCheckBox::clicked, configManager, &SpectacleConfig::setIncludeDecorationsChecked);

    mCaptureTransientOnly = new QCheckBox(i18n("Capture the current pop-up only"), this);
    mCaptureTransientOnly->setToolTip(i18n("Capture only the current pop-up window (like a menu, tooltip etc).\n"
                                           "If disabled, the pop-up is captured along with the parent window"));
    mCaptureTransientOnly->setEnabled(false);
    connect(mCaptureTransientOnly, &QCheckBox::clicked, configManager, &SpectacleConfig::setCaptureTransientWindowOnlyChecked);

    mQuitAfterSaveOrCopy = new QCheckBox(i18n("Quit after Save or Copy"), this);
    mQuitAfterSaveOrCopy->setToolTip(i18n("Quit Spectacle after saving or copying the image"));
    connect(mQuitAfterSaveOrCopy, &QCheckBox::clicked, configManager, &SpectacleConfig::setQuitAfterSaveOrCopyChecked);

    mContentOptionsForm = new QVBoxLayout;
    mContentOptionsForm->addWidget(mMousePointer);
    mContentOptionsForm->addWidget(mWindowDecorations);
    mContentOptionsForm->addWidget(mCaptureTransientOnly);
    mContentOptionsForm->addWidget(mQuitAfterSaveOrCopy);
    mContentOptionsForm->setContentsMargins(24, 0, 0, 0);

    // the take a new screenshot button

    mTakeScreenshotButton = new QPushButton(i18n("Take a New Screenshot"), this);
    mTakeScreenshotButton->setIcon(QIcon::fromTheme(QStringLiteral("spectacle")));
    mTakeScreenshotButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    mTakeScreenshotButton->setFocus();
    connect(mTakeScreenshotButton, &QPushButton::clicked, this, &KSWidget::newScreenshotClicked);

    QShortcut *shortcut = new QShortcut(QKeySequence(QKeySequence::New), mTakeScreenshotButton);
    auto clickFunc = [&]() {
        mTakeScreenshotButton->animateClick(100);
        QTimer::singleShot(100, mTakeScreenshotButton, &QPushButton::click);
    };
    connect(shortcut, &QShortcut::activated, clickFunc);

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

    mMainLayout = new QGridLayout(this);
    mMainLayout->addWidget(mImageWidget, 0, 0, 1, 1);
    mMainLayout->addLayout(mRightLayout, 0, 1, 1, 1);
    mMainLayout->setColumnMinimumWidth(0, 320);
    mMainLayout->setColumnMinimumWidth(1, 320);

    // and read in the saved checkbox states and capture mode indices

    mMousePointer->setChecked         (configManager->includePointerChecked());
    mWindowDecorations->setChecked    (configManager->includeDecorationsChecked());
    mCaptureOnClick->setChecked       (configManager->onClickChecked());
    mCaptureTransientOnly->setChecked (configManager->captureTransientWindowOnlyChecked());
    mQuitAfterSaveOrCopy->setChecked  (configManager->quitAfterSaveOrCopyChecked());
    mCaptureArea->setCurrentIndex     (configManager->captureMode());
    mDelayMsec->setValue              (configManager->captureDelay());

    // done
}

int KSWidget::imagePaddingWidth() const
{
    int rightLayoutLeft = 0;
    int rightLayoutRight = 0;
    int mainLayoutRight = 0;

    mRightLayout->getContentsMargins(&rightLayoutLeft, nullptr, &rightLayoutRight, nullptr);
    mMainLayout->getContentsMargins(nullptr, nullptr, &mainLayoutRight, nullptr);

    int paddingWidth = (rightLayoutLeft + rightLayoutRight + mainLayoutRight);
    paddingWidth += mRightLayout->contentsRect().width();
    paddingWidth += 2 * SpectacleImage::SHADOW_RADIUS; // image drop shadow

    return paddingWidth;
}

// public slots

void KSWidget::setScreenshotPixmap(const QPixmap &pixmap)
{
    mImageWidget->setScreenshot(pixmap);
}

void KSWidget::disableOnClick()
{
    mCaptureOnClick->setEnabled(false);
    mDelayMsec->setEnabled(true);
}

// private slots

void KSWidget::newScreenshotClicked()
{
    int delay = mCaptureOnClick->isChecked() ? -1 : (mDelayMsec->value() * 1000);
    ImageGrabber::GrabMode mode = static_cast<ImageGrabber::GrabMode>(mCaptureArea->currentData().toInt());
    if (mode == ImageGrabber::WindowUnderCursor && !(mCaptureTransientOnly->isChecked())) {
        mode = ImageGrabber::TransientWithParent;
    }

    emit newScreenshotRequest(mode, delay, mMousePointer->isChecked(), mWindowDecorations->isChecked());
}

void KSWidget::onClickStateChanged(int state)
{
    if (state == Qt::Checked) {
        mDelayMsec->setEnabled(false);
    } else if (state == Qt::Unchecked) {
        mDelayMsec->setEnabled(true);
    }
}

void KSWidget::captureModeChanged(int index)
{
    SpectacleConfig::instance()->setCaptureMode(index);

    ImageGrabber::GrabMode mode = static_cast<ImageGrabber::GrabMode>(mCaptureArea->itemData(index).toInt());
    switch (mode) {
    case ImageGrabber::WindowUnderCursor:
        mWindowDecorations->setEnabled(true);
        mCaptureTransientOnly->setEnabled(true);
        break;
    case ImageGrabber::ActiveWindow:
        mWindowDecorations->setEnabled(true);
        mCaptureTransientOnly->setEnabled(false);
        break;
    default:
        mWindowDecorations->setEnabled(false);
        mCaptureTransientOnly->setEnabled(false);
    }
}
