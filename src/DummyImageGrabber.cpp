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

void DummyImageGrabber::blendCursorImage(int x, int y, int width, int height)
{
    return;
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
