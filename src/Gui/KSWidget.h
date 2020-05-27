/* This file is part of Spectacle, the KDE screenshot utility
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

#pragma once

#include <QWidget>
#include <QPixmap>

#include "SpectacleCommon.h"
#include "Platforms/Platform.h"

class QAction;
class QGridLayout;
class QHBoxLayout;
class QVBoxLayout;
class QFormLayout;
class QComboBox;
class QCheckBox;
class QLabel;

class KConfigDialogManager;

class KSImageWidget;
class ProgressButton;
class SmartSpinBox;

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
};
