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
