#ifndef X11IMAGEGRABBER_H
#define X11IMAGEGRABBER_H

#include <QX11Info>
#include <QString>
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QCursor>
#include <QPoint>
#include <QRect>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QXmlStreamReader>

#include <KWindowSystem>
#include <KScreen/Config>
#include <KScreen/GetConfigOperation>
#include <KScreen/Screen>
#include <KScreen/Output>

#include "ImageGrabber.h"

struct X11GeometryInfo
{
    long unsigned int root;
    int               x;
    int               y;
    unsigned int      height;
    unsigned int      width;
    unsigned int      border_width;
    unsigned int      depth;
};

class X11ImageGrabber : public ImageGrabber
{
    Q_OBJECT

    public:

    explicit X11ImageGrabber(QObject * parent = 0);
    ~X11ImageGrabber();

    protected:

    void blendCursorImage(int x, int y, int width, int height);
    void grabFullScreen();
    void grabCurrentScreen();
    void grabActiveWindow();

    private slots:

    void KWinDBusScreenshotHelper(quint64 winId);
    void KScreenCurrentMonitorScreenshotHelper(KScreen::ConfigOperation *op);

    private:

    bool KWinDBusScreenshotAvailable();

    QPixmap processXImage24Bit(void *ximage_d);
    QPixmap processXImage30Bit(void *ximage_d);
    QPixmap processXImage32Bit(void *ximage_d);
    QPixmap getWindowPixmap(quint64 winId);

    KScreen::GetConfigOperation *co;
};

#endif // X11IMAGEGRABBER_H
