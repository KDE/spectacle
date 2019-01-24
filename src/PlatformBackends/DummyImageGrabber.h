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

#ifndef DUMMYIMAGEGRABBER_H
#define DUMMYIMAGEGRABBER_H

#include <QObject>
#include <QPixmap>

#include "ImageGrabber.h"

class DummyImageGrabber : public ImageGrabber
{
    Q_OBJECT

    public:

    explicit DummyImageGrabber(QObject *parent = nullptr);
    ~DummyImageGrabber() override;

    QVector<ImageGrabber::GrabMode> supportedModes() const override { return {FullScreen, CurrentScreen, ActiveWindow, WindowUnderCursor, TransientWithParent, RectangularRegion}; }
    bool onClickGrabSupported() const override;

    protected:

    QPixmap blendCursorImage(const QPixmap &pixmap, int x, int y, int width, int height) override;
    void grabFullScreen()          override;
    void grabCurrentScreen()       override;
    void grabActiveWindow()        override;
    void grabRectangularRegion()   override;
    void grabWindowUnderCursor()   override;
    void grabTransientWithParent() override;
};

#endif // DUMMYIMAGEGRABBER_H
