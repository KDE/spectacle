/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QWidget>
#include <QPixmap>

#include "SpectacleCommon.h"
#include "Platforms/Platform.h"
#include "Config.h"

class QAction;
class QGridLayout;
class QHBoxLayout;
class QVBoxLayout;
class QFormLayout;
class QComboBox;
class QCheckBox;
class QLabel;
class KConfigDialogManager;
class QPushButton;
class QStackedLayout;

class KSImageWidget;
class ProgressButton;
class SmartSpinBox;

namespace kImageAnnotator {
    class KImageAnnotator;
}

class KSWidget : public QWidget
{
    Q_OBJECT

	public:

    explicit KSWidget(Platform::GrabModes theGrabModes, QWidget *parent = nullptr);
    virtual ~KSWidget() = default;


    enum class State {
        TakeNewScreenshot,
        Cancel
    };

    int imagePaddingWidth() const;

	Q_SIGNALS:

    void dragInitiated();
    void newScreenshotRequest(Spectacle::CaptureMode theCaptureMode, int theCaptureDelat, bool theIncludePointer, bool theIncludeDecorations);
    void screenshotCanceled();

	public Q_SLOTS:

    void setScreenshotPixmap(const QPixmap &thePixmap);
    void lockOnClickDisabled();
    void lockOnClickEnabled();
    void setButtonState(KSWidget::State state);
    void setProgress(double progress);

#ifdef KIMAGEANNOTATOR_FOUND
    void showAnnotator();
    void hideAnnotator();
#endif

    private Q_SLOTS:

    void newScreenshotClicked();
    void onClickStateChanged(int theState);
    void captureModeChanged(int theIndex);

	private:

    QGridLayout   *mMainLayout                   { nullptr };
    QHBoxLayout   *mDelayLayout                  { nullptr };
    QVBoxLayout   *mRightLayout                  { nullptr };
    QFormLayout   *mCaptureModeForm              { nullptr };
    QVBoxLayout   *mContentOptionsForm           { nullptr };
    KSImageWidget *mImageWidget                  { nullptr };
    ProgressButton*mTakeScreenshotButton;
    QComboBox     *mCaptureArea                  { nullptr };
    SmartSpinBox  *mDelayMsec                    { nullptr };
    QCheckBox     *mCaptureOnClick               { nullptr };
    QCheckBox     *mMousePointer                 { nullptr };
    QCheckBox     *mWindowDecorations            { nullptr };
    QCheckBox     *mCaptureTransientOnly         { nullptr };
    QCheckBox     *mQuitAfterSaveOrCopy          { nullptr };
    QLabel        *mCaptureModeLabel             { nullptr };
    QLabel        *mContentOptionsLabel          { nullptr };
    bool           mTransientWithParentAvailable { false };
    QAction       *mTakeNewScreenshotAction;
    QAction       *mCancelAction;
    KConfigDialogManager *mConfigManager;
    QStackedLayout *mStack                       { nullptr };
    QWidget *placeHolder;
#ifdef KIMAGEANNOTATOR_FOUND
    kImageAnnotator::KImageAnnotator *mAnnotator { nullptr };
#endif
};
