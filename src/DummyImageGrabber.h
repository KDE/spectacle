#ifndef DUMMYIMAGEGRABBER_H
#define DUMMYIMAGEGRABBER_H

#include <QObject>
#include <QPixmap>

#include "ImageGrabber.h"

class DummyImageGrabber : public ImageGrabber
{
    Q_OBJECT

    public:

    explicit DummyImageGrabber(QObject *parent = 0);
    ~DummyImageGrabber();

    bool onClickGrabSupported() const Q_DECL_OVERRIDE;

    protected:

    void blendCursorImage(int x, int y, int width, int height) Q_DECL_OVERRIDE;
    void grabFullScreen() Q_DECL_OVERRIDE;
    void grabCurrentScreen() Q_DECL_OVERRIDE;
    void grabActiveWindow() Q_DECL_OVERRIDE;
    void grabRectangularRegion() Q_DECL_OVERRIDE;
};

#endif // DUMMYIMAGEGRABBER_H
