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

#ifndef IMAGEGRABBER_H
#define IMAGEGRABBER_H

#include <QList>
#include <QPixmap>
#include <QScreen>
#include <QDesktopWidget>
#include <QApplication>

#include "CropScreenshotGrabber.h"

class ImageGrabber : public QObject
{
    Q_OBJECT
    Q_ENUMS(GrabMode)

    Q_PROPERTY(QPixmap pixmap READ pixmap NOTIFY pixmapChanged)
    Q_PROPERTY(bool capturePointer READ capturePointer WRITE setCapturePointer NOTIFY capturePointerChanged)
    Q_PROPERTY(bool captureDecorations READ captureDecorations WRITE setCaptureDecorations NOTIFY captureDecorationsChanged)
    Q_PROPERTY(GrabMode grabMode READ grabMode WRITE setGrabMode NOTIFY grabModeChanged)

    public:

    enum GrabMode {
        InvalidChoice     = -1,
        FullScreen        = 0,
        CurrentScreen     = 1,
        ActiveWindow      = 2,
        RectangularRegion = 3
    };

    explicit ImageGrabber(QObject *parent = 0);
    ~ImageGrabber();

    QPixmap pixmap() const;
    bool capturePointer() const;
    bool captureDecorations() const;
    GrabMode grabMode() const;

    void setCapturePointer(const bool newCapturePointer);
    void setCaptureDecorations(const bool newCaptureDecorations);
    void setGrabMode(const GrabMode newGrabMode);

    signals:

    void pixmapChanged(const QPixmap pixmap);
    void imageGrabFailed();
    void capturePointerChanged(bool capturePointer);
    void captureDecorationsChanged(bool captureDecorations);
    void grabModeChanged(GrabMode grabMode);

    public slots:

    virtual void doImageGrab();
    virtual void doOnClickGrab();

    protected:

    virtual void grabFullScreen() = 0;
    virtual void grabCurrentScreen() = 0;
    virtual void grabActiveWindow() = 0;
    virtual void grabRectangularRegion() = 0;
    virtual void blendCursorImage(int x, int y, int width, int height) = 0;

    bool     mCapturePointer;
    bool     mCaptureDecorations;
    GrabMode mGrabMode;
    QPixmap  mPixmap;
};

#endif // IMAGEGRABBER_H
