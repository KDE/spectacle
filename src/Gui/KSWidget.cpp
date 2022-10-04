/*
 *  SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KSWidget.h"
#include "spectacle_gui_debug.h"

#include "CaptureAreaComboBox.h"
#include "ExportManager.h"
#include "KSImageWidget.h"
#include "ProgressButton.h"
#include "SmartSpinBox.h"
#include "settings.h"

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
#include <QGraphicsOpacityEffect>

KSWidget::KSWidget(Platform::GrabModes theGrabModes, QWidget *parent)
    : QWidget(parent)
    , mImageWidget(new KSImageWidget(this))
{
    mStack = new QStackedLayout(this);

    // we'll init the widget that holds the image first
    mImageWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(mImageWidget, &KSImageWidget::dragInitiated, this, &KSWidget::dragInitiated);

    // the capture mode options first
    mCaptureModeLabel = new QLabel(i18n("<b>Capture Mode</b>"), this);
    mCaptureArea = new CaptureAreaComboBox(theGrabModes, this);

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
    mCaptureTransientOnly->setToolTip(
        i18n("Capture only the current pop-up window (like a menu, tooltip etc).\n"
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
        Q_EMIT screenshotCanceled();
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

    mPlaceholderLabel = new QLabel;

    QFont placeholderLabelFont;
    // To match the size of a level 2 Heading/KTitleWidget
    placeholderLabelFont.setPointSize(qRound(placeholderLabelFont.pointSize() * 1.3));
    mPlaceholderLabel->setFont(placeholderLabelFont);
    mPlaceholderLabel->setTextInteractionFlags(Qt::NoTextInteraction);
    mPlaceholderLabel->setWordWrap(true);
    mPlaceholderLabel->setAlignment(Qt::AlignCenter);
    mPlaceholderLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // Match opacity of QML placeholder label component
    auto *effect = new QGraphicsOpacityEffect(mPlaceholderLabel);
    effect->setOpacity(0.5);
    mPlaceholderLabel->setGraphicsEffect(effect);

    mMainLayout = new QGridLayout();

    mMainLayout->addWidget(mPlaceholderLabel, 0, 0, 1, 1);
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
}

int KSWidget::imagePaddingWidth() const
{
    int lRightLayoutLeft = 0;
    int lRightLayoutRight = 0;
    int lMainLayoutRight = 0;

    mRightLayout->getContentsMargins(&lRightLayoutLeft, nullptr, &lRightLayoutRight, nullptr);
    mMainLayout->getContentsMargins(nullptr, nullptr, &lMainLayoutRight, nullptr);

    int lPaddingWidth = (lRightLayoutLeft + lRightLayoutRight + lMainLayoutRight);
    lPaddingWidth += mRightLayout->contentsRect().width();
    lPaddingWidth += 2 * SpectacleImage::SHADOW_RADIUS; // image drop shadow

    return lPaddingWidth;
}

bool KSWidget::isScreenshotSet() const
{
    return mImageWidget->isPixmapSet();
}

// public slots

void KSWidget::showPlaceholderText(const QString &label)
{
    mImageWidget->hide();
    mPlaceholderLabel->setText(label);
    mPlaceholderLabel->show();
}

void KSWidget::setScreenshotPixmap(const QPixmap &thePixmap)
{
    if (mPlaceholderLabel->isVisible()) {
        mPlaceholderLabel->hide();
    }
    mImageWidget->show();
    mImageWidget->setScreenshot(thePixmap);
    Q_EMIT screenshotPixmapSet();
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
    mCaptureOnClick->hide();
    mDelayMsec->setEnabled(true);
}

// private slots

void KSWidget::newScreenshotClicked()
{
    int lDelay = mCaptureOnClick->isChecked() ? -1 : (mDelayMsec->value() * 1000);
    auto lMode = mCaptureArea->currentCaptureMode();
    if (mTransientWithParentAvailable && lMode == Spectacle::CaptureMode::WindowUnderCursor && !(mCaptureTransientOnly->isChecked())) {
        lMode = Spectacle::CaptureMode::TransientWithParent;
    }
    setButtonState(State::Cancel);
    Q_EMIT newScreenshotRequest(lMode, lDelay, mMousePointer->isChecked(), mWindowDecorations->isChecked());
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
    auto captureMode = mCaptureArea->captureModeForIndex(theIndex);
    switch (captureMode) {
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
    case Spectacle::CaptureMode::AllScreensScaled:
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
    if (!mAnnotator) {
        mAnnotator = new kImageAnnotator::KImageAnnotator();
        mAnnotator->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        mAnnotator->setCanvasColor(QColor(255, 255, 255, 0));
        mAnnotator->setSaveToolSelection(true);
#ifdef KIMAGEANNOTATOR_HAS_EXTRA_TOOLS
        mAnnotator->setControlsWidgetVisible(true);
#endif

        mStack->addWidget(mAnnotator);
    }

    mStack->setCurrentIndex(1);
    QPixmap px = ExportManager::instance()->pixmap();
    px.setDevicePixelRatio(qApp->devicePixelRatio());
    mAnnotator->loadImage(px);
}

void KSWidget::hideAnnotator()
{
    if (!mAnnotator) {
        return;
    }

    mStack->setCurrentIndex(0);
    QImage image = mAnnotator->image();
    QPixmap px = QPixmap::fromImage(image);
    setScreenshotPixmap(px);
    ExportManager::instance()->setPixmap(px);
}

QSize KSWidget::sizeHintWhenAnnotating()
{
    if (!mAnnotator) {
        return QSize();
    }

    /*
     * when using Wayland only maximization shall be used and return value ignored.
     * We return it anyway for other possible use-cases.
     * Reason: Wayland doesn't allow moving of the window hence a too big window
     * of spectacle would 'overflow' outside the display.
     */

#ifdef KIMAGEANNOTATOR_HAS_FIXED_SIZEHINT
    return mAnnotator->sizeHint();
#endif

    if (qApp->devicePixelRatio() == 1) {
        return mAnnotator->sizeHint();
    } else {
        // if scaling is >100%, than `image().size() > sizeHint()` calculates an bigger value
        // unless KIMAGEANNOTATOR_HAS_FIXED_SIZEHINT is set
        return mAnnotator->image().size();
    }
}

#endif
