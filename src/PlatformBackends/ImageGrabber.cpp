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

#include "ImageGrabber.h"

ImageGrabber::ImageGrabber(QObject *parent) :
    QObject(parent),
    mCapturePointer(false),
    mCaptureDecorations(true),
    mGrabMode(InvalidChoice),
    mPixmap(QPixmap())
{
}

ImageGrabber::~ImageGrabber()
{
}

//

bool ImageGrabber::onClickGrabSupported() const
{
    return false;
}

// Q_PROPERTY Stuff

QPixmap ImageGrabber::pixmap() const
{
    return mPixmap;
}

bool ImageGrabber::capturePointer() const
{
    return mCapturePointer;
}

bool ImageGrabber::captureDecorations() const
{
    return mCaptureDecorations;
}

ImageGrabber::GrabMode ImageGrabber::grabMode() const
{
    return mGrabMode;
}

void ImageGrabber::setCapturePointer(const bool newCapturePointer)
{
    mCapturePointer = newCapturePointer;
}

void ImageGrabber::setCaptureDecorations(const bool newCaptureDecorations)
{
    mCaptureDecorations = newCaptureDecorations;
}

void ImageGrabber::setGrabMode(const GrabMode newGrabMode)
{
    mGrabMode = newGrabMode;
}

// Slots

void ImageGrabber::doOnClickGrab()
{
    return doImageGrab();
}

void ImageGrabber::doImageGrab()
{
    switch(mGrabMode) {
    case FullScreen:
        grabFullScreen();
        break;
    case CurrentScreen:
        grabCurrentScreen();
        break;
    case ActiveWindow:
        grabActiveWindow();
        break;
    case WindowUnderCursor:
        grabWindowUnderCursor();
        break;
    case TransientWithParent:
        grabTransientWithParent();
        break;
    case RectangularRegion:
        grabRectangularRegion();
        break;
    case InvalidChoice:
    default:
        emit imageGrabFailed();
        return;
    }
}
