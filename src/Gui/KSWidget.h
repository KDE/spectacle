/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QPixmap>
#include <QWidget>

#include "Config.h"
#include "Platforms/Platform.h"
#include "SpectacleCommon.h"

class QAction;
class QGridLayout;
class QHBoxLayout;
class QVBoxLayout;
class QFormLayout;
class QComboBox;
class QCheckBox;
class QLabel;
class KConfigDialogManager;
class QStackedLayout;

class CaptureAreaComboBox;
class KSImageWidget;
class ProgressButton;
class SmartSpinBox;

namespace kImageAnnotator
{
class KImageAnnotator;
}

class KSWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KSWidget(Platform::GrabModes theGrabModes, QWidget *parent = nullptr);
    ~KSWidget() override = default;

    enum class State { TakeNewScreenshot, Cancel };

    int imagePaddingWidth() const;
    bool isScreenshotSet() const;

Q_SIGNALS:

    void dragInitiated();
    void newScreenshotRequest(Spectacle::CaptureMode theCaptureMode, int theCaptureDelat, bool theIncludePointer, bool theIncludeDecorations);
    void screenshotCanceled();
    void screenshotPixmapSet();

public Q_SLOTS:

    void showPlaceholderText(const QString &label);
    void setScreenshotPixmap(const QPixmap &thePixmap);
    void lockOnClickDisabled();
    void lockOnClickEnabled();
    void setButtonState(KSWidget::State state);
    void setProgress(double progress);

#ifdef KIMAGEANNOTATOR_FOUND
    void showAnnotator();
    void hideAnnotator();
    QSize sizeHintWhenAnnotating();
#endif

private Q_SLOTS:

    void newScreenshotClicked();
    void onClickStateChanged(int theState);
    void captureModeChanged(int theIndex);

private:
    QGridLayout *mMainLayout{nullptr};
    QHBoxLayout *mDelayLayout{nullptr};
    QVBoxLayout *mRightLayout{nullptr};
    QFormLayout *mCaptureModeForm{nullptr};
    QVBoxLayout *mContentOptionsForm{nullptr};
    KSImageWidget *const mImageWidget;
    ProgressButton *mTakeScreenshotButton;
    CaptureAreaComboBox *mCaptureArea{nullptr};
    SmartSpinBox *mDelayMsec{nullptr};
    QCheckBox *mCaptureOnClick{nullptr};
    QCheckBox *mMousePointer{nullptr};
    QCheckBox *mWindowDecorations{nullptr};
    QCheckBox *mCaptureTransientOnly{nullptr};
    QCheckBox *mQuitAfterSaveOrCopy{nullptr};
    QLabel *mCaptureModeLabel{nullptr};
    QLabel *mContentOptionsLabel{nullptr};
    QLabel *mPlaceholderLabel { nullptr };
    bool mTransientWithParentAvailable{false};
    QAction *mTakeNewScreenshotAction{nullptr};
    QAction *mCancelAction{nullptr};
    KConfigDialogManager *mConfigManager{nullptr};
    QStackedLayout *mStack{nullptr};
    QWidget *placeHolder{nullptr};

#ifdef KIMAGEANNOTATOR_FOUND
    kImageAnnotator::KImageAnnotator *mAnnotator{nullptr};
#endif
};
