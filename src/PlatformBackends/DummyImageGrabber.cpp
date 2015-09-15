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

#include "DummyImageGrabber.h"

DummyImageGrabber::DummyImageGrabber(QObject *parent):
    ImageGrabber(parent)
{}

DummyImageGrabber::~DummyImageGrabber()
{}

bool DummyImageGrabber::onClickGrabSupported() const
{
    return false;
}

QPixmap DummyImageGrabber::blendCursorImage(const QPixmap &pixmap, int x, int y, int width, int height)
{
    Q_UNUSED(pixmap);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(width);
    Q_UNUSED(height);
    return QPixmap();
}

void DummyImageGrabber::grabFullScreen()
{
    emit pixmapChanged(QPixmap());
}

void DummyImageGrabber::grabCurrentScreen()
{
    emit pixmapChanged(QPixmap());
}

void DummyImageGrabber::grabActiveWindow()
{
    emit pixmapChanged(QPixmap());
}

void DummyImageGrabber::grabRectangularRegion()
{
    emit pixmapChanged(QPixmap());
}

void DummyImageGrabber::grabWindowUnderCursor()
{
    emit pixmapChanged(QPixmap());
}

void DummyImageGrabber::grabTransientWithParent()
{
    emit pixmapChanged(QPixmap());
}
