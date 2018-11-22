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
    ~DummyImageGrabber() Q_DECL_OVERRIDE;

    QVector<ImageGrabber::GrabMode> supportedModes() const override { return {FullScreen, CurrentScreen, ActiveWindow, WindowUnderCursor, TransientWithParent, RectangularRegion}; }
    bool onClickGrabSupported() const Q_DECL_OVERRIDE;

    protected:

    QPixmap blendCursorImage(const QPixmap &pixmap, int x, int y, int width, int height) Q_DECL_OVERRIDE;
    void grabFullScreen()          Q_DECL_OVERRIDE;
    void grabCurrentScreen()       Q_DECL_OVERRIDE;
    void grabActiveWindow()        Q_DECL_OVERRIDE;
    void grabRectangularRegion()   Q_DECL_OVERRIDE;
    void grabWindowUnderCursor()   Q_DECL_OVERRIDE;
    void grabTransientWithParent() Q_DECL_OVERRIDE;
};

#endif // DUMMYIMAGEGRABBER_H
