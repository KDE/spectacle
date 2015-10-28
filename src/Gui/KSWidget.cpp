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

KSWidget::KSWidget(QWidget *parent) :
    QWidget(parent)
{
    // we'll init the widget that holds the image first

    mImageWidget = new KSImageWidget(this);
    mImageWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(mImageWidget, &KSImageWidget::dragInitiated, this, &KSWidget::dragInitiated);

    // the capture mode options first

    mCaptureModeLabel = new QLabel(this);
    mCaptureModeLabel->setText(i18n("<b>Capture Mode</b>"));

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
    mDelayMsec->setSuffix(i18n(" seconds"));
    mDelayMsec->setMinimumWidth(160);
    connect(mDelayMsec, static_cast<void (SmartSpinBox::*)(qreal)>(&SmartSpinBox::valueChanged), this, &KSWidget::captureDelayChanged);

    mCaptureOnClick = new QCheckBox(this);
    mCaptureOnClick->setText(i18n("On Click"));
    mCaptureOnClick->setToolTip(i18n("Wait for a mouse click before capturing the screenshot image"));
    connect(mCaptureOnClick, &QCheckBox::stateChanged, this, &KSWidget::onClickStateChanged);
    connect(mCaptureOnClick, &QCheckBox::stateChanged, this, &KSWidget::checkboxStatesChanged);

    mDelayLayout = new QHBoxLayout;
    mDelayLayout->addWidget(mDelayMsec);
    mDelayLayout->addWidget(mCaptureOnClick);

    mCaptureModeForm = new QFormLayout;
    mCaptureModeForm->addRow(i18n("Area:"), mCaptureArea);
    mCaptureModeForm->addRow(i18n("Delay:"), mDelayLayout);
    mCaptureModeForm->setContentsMargins(24, 0, 0, 0);

    // the content options (mouse pointer, window decorations)

    mContentOptionsLabel = new QLabel(this);
    mContentOptionsLabel->setText(i18n("<b>Content Options</b>"));

    mMousePointer = new QCheckBox(this);
    mMousePointer->setText(i18n("Include mouse pointer"));
    mMousePointer->setToolTip(i18n("Show the mouse cursor in the screeenshot image"));
    connect(mMousePointer, &QCheckBox::stateChanged, this, &KSWidget::checkboxStatesChanged);

    mWindowDecorations = new QCheckBox(this);
    mWindowDecorations->setText(i18n("Include window titlebar and borders"));
    mWindowDecorations->setToolTip(i18n("Show the window title bar, the minimize/maximize/close buttons, and the window border"));
    mWindowDecorations->setEnabled(false);
    connect(mWindowDecorations, &QCheckBox::stateChanged, this, &KSWidget::checkboxStatesChanged);

    mCaptureTransientOnly = new QCheckBox(this);
    mCaptureTransientOnly->setText(i18n("Capture the current pop-up only"));
    mCaptureTransientOnly->setToolTip(i18n("Capture only the current pop-up window (like a menu, tooltip etc). "
                                           "If this is not enabled, the pop-up is captured along with the parent window"));
    mCaptureTransientOnly->setEnabled(false);
    connect(mCaptureTransientOnly, &QCheckBox::stateChanged, this, &KSWidget::checkboxStatesChanged);

    mContentOptionsForm = new QVBoxLayout;
    mContentOptionsForm->addWidget(mMousePointer);
    mContentOptionsForm->addWidget(mWindowDecorations);
    mContentOptionsForm->addWidget(mCaptureTransientOnly);
    mContentOptionsForm->setSpacing(16);
    mContentOptionsForm->setContentsMargins(24, 0, 0, 0);

    // the take new screenshot button

    mTakeScreenshotButton = new QPushButton(this);
    mTakeScreenshotButton->setText(i18n("Take New Screenshot"));
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
    mRightLayout->addSpacing(10);
    mRightLayout->addLayout(mCaptureModeForm);
    mRightLayout->addStretch(1);
    mRightLayout->addWidget(mContentOptionsLabel);
    mRightLayout->addSpacing(10);
    mRightLayout->addLayout(mContentOptionsForm);
    mRightLayout->addStretch(10);
    mRightLayout->addWidget(mTakeScreenshotButton, 1, Qt::AlignHCenter);
    mRightLayout->setContentsMargins(20, 0, 0, 10);

    mMainLayout = new QGridLayout(this);
    mMainLayout->addWidget(mImageWidget, 0, 0, 1, 1);
    mMainLayout->addLayout(mRightLayout, 0, 1, 1, 1);
    mMainLayout->setColumnMinimumWidth(0, 400);
    mMainLayout->setColumnMinimumWidth(1, 400);

    // and read in the saved checkbox states and capture mode indices

    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("spectaclerc"));
    KConfigGroup guiConfig(config, "GuiConfig");

    mMousePointer->setChecked(guiConfig.readEntry("includePointer", true));
    mWindowDecorations->setChecked(guiConfig.readEntry("includeDecorations", true));
    mCaptureOnClick->setChecked(guiConfig.readEntry("waitCaptureOnClick", false));
    mCaptureTransientOnly->setChecked(guiConfig.readEntry("transientOnly", false));
    mCaptureArea->setCurrentIndex(guiConfig.readEntry("captureModeIndex", 0));
    mDelayMsec->setValue(guiConfig.readEntry("captureDelay", (qreal)0));

    // done
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

void KSWidget::checkboxStatesChanged(int state)
{
    Q_UNUSED(state);

    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("spectaclerc"));
    KConfigGroup guiConfig(config, "GuiConfig");

    guiConfig.writeEntry("includePointer",     mMousePointer->isChecked());
    guiConfig.writeEntry("includeDecorations", mWindowDecorations->isChecked());
    guiConfig.writeEntry("waitCaptureOnClick", mCaptureOnClick->isChecked());
    guiConfig.writeEntry("transientOnly",      mCaptureTransientOnly->isChecked());
    guiConfig.sync();
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
    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("spectaclerc"));
    KConfigGroup guiConfig(config, "GuiConfig");

    guiConfig.writeEntry("captureModeIndex", index);
    guiConfig.sync();

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

void KSWidget::captureDelayChanged(qreal value)
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("spectaclerc"));
    KConfigGroup guiConfig(config, "GuiConfig");

    guiConfig.writeEntry("captureDelay", value);
    guiConfig.sync();
}
