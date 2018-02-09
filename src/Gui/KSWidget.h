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

#ifndef KSWIDGET_H
#define KSWIDGET_H

#include <QWidget>
#include <QPixmap>

#include "PlatformBackends/ImageGrabber.h"

class QGridLayout;
class QHBoxLayout;
class QVBoxLayout;
class QFormLayout;
class QComboBox;
class QCheckBox;
class QLabel;
class QPushButton;

class KSImageWidget;
class SmartSpinBox;

class KSWidget : public QWidget
{
    Q_OBJECT

	public:

    explicit KSWidget(QWidget *parent = 0);

    int imagePaddingWidth() const;

	signals:

    void dragInitiated();
    void newScreenshotRequest(ImageGrabber::GrabMode mode, int captureDelay, bool capturePointer, bool captureDecorations);

	public slots:

    void setScreenshotPixmap(const QPixmap &pixmap);
    void disableOnClick();

    private slots:

    void newScreenshotClicked();
    void onClickStateChanged(int state);
    void captureModeChanged(int index);

	private:

    QGridLayout   *mMainLayout;
    QHBoxLayout   *mDelayLayout;
    QVBoxLayout   *mRightLayout;
    QFormLayout   *mCaptureModeForm;
    QVBoxLayout   *mContentOptionsForm;
    KSImageWidget *mImageWidget;
    QPushButton   *mTakeScreenshotButton;
    QComboBox     *mCaptureArea;
    SmartSpinBox  *mDelayMsec;
    QCheckBox     *mCaptureOnClick;
    QCheckBox     *mMousePointer;
    QCheckBox     *mWindowDecorations;
    QCheckBox     *mCaptureTransientOnly;
    QLabel        *mCaptureModeLabel;
    QLabel        *mContentOptionsLabel;
};

#endif // KSWIDGET_H
