/*
 *  Copyright (C) 2016 Martin Graesslin <mgraesslin@kde.org>
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
#ifndef KWINWAYLANDIMAGEGRABBER_H
#define KWINWAYLANDIMAGEGRABBER_H

#include "ImageGrabber.h"

class KWinWaylandImageGrabber : public ImageGrabber
{
    Q_OBJECT

    public:

    explicit KWinWaylandImageGrabber(QObject * parent = nullptr);
    ~KWinWaylandImageGrabber() override;

    QVector<ImageGrabber::GrabMode> supportedModes() const override { return {FullScreen, CurrentScreen, /*ActiveWindow, */WindowUnderCursor, TransientWithParent/*, RectangularRegion*/}; }
    bool onClickGrabSupported() const override;

    protected:

    void grabFullScreen()          override;
    void grabCurrentScreen()       override;
    void grabActiveWindow()        override;
    void grabRectangularRegion()   override;
    void grabWindowUnderCursor()   override;
    void grabTransientWithParent() override;
    QPixmap blendCursorImage(const QPixmap &pixmap, int x, int y, int width, int height) override;

    private:

    void startReadImage(int readPipe);
    enum class Mode {
        Window,
        CurrentScreen,
        FullScreen
    };
    template <typename T>
    void callDBus(Mode mode, int writeFd, T argument);
    template <typename T>
    void grab(Mode mode, T argument);
};

#endif
