#include "X11ImageGrabber.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>

X11ImageGrabber::X11ImageGrabber(QObject *parent) :
    ImageGrabber(parent),
    co(nullptr)
{}

X11ImageGrabber::~X11ImageGrabber()
{}

// image conversion routines

QPixmap X11ImageGrabber::processXImage30Bit(void *ximage_d)
{
    XImage *ximage = (XImage *)ximage_d;
    quint8 *imageData = new quint8[(ximage->height * ximage->width) * 4];

    for (qint64 x = 0; x < ximage->width; x++) {
        for (qint64 y = 0; y < ximage->height; y++) {
            quint32 pixel = XGetPixel(ximage, x, y);

            imageData[((x + ximage->width * y) * 4)]     = (pixel >> 22) & 0xff;
            imageData[((x + ximage->width * y) * 4) + 1] = (pixel >> 12) & 0xff;
            imageData[((x + ximage->width * y) * 4) + 2] = (pixel >>  2) & 0xff;
            imageData[((x + ximage->width * y) * 4) + 3] = 0xff;
        }
    }

    QImage image((quint8 *)imageData, ximage->width, ximage->height, QImage::Format_RGBA8888);
    QPixmap pixmap = QPixmap::fromImage(image);

    delete imageData;
    return pixmap;
}

QPixmap X11ImageGrabber::processXImage32Bit(void *ximage_d)
{
    XImage *ximage = (XImage *)ximage_d;

    QImage image((quint8 *)ximage->data, ximage->width, ximage->height, QImage::Format_ARGB32_Premultiplied);
    return QPixmap::fromImage(image);
}

// utility functions

void X11ImageGrabber::blendCursorImage(int x, int y, int width, int height)
{
    // credit for how to do this goes here
    // https://msnkambule.wordpress.com/2010/04/09/capturing-a-screenshot-showing-mouse-cursor-in-kde/

    // first we get the cursor position, compute the co-ordinates of the region
    // of the screen we're grabbing, and see if the cursor is actually visible in
    // the region

    QPoint cursorPos = QCursor::pos();
    QRect screenRect(x, y, width, height);

    if (!(screenRect.contains(cursorPos))) {
        return;
    }

    // now we can get the image and start processing

    XFixesCursorImage *xfCursorImage = XFixesGetCursorImage(QX11Info::display());

    // turns out we have a slight problem. xfCursorImage->pixels is an array of
    // unsigned longs, which is not portable. An unsigned long is 32 bits on a 32bit
    // system, and 64 bits on *some* 64 bit systems. What we'll do is attempt to
    // convert the 64 bit unsigned longs into a 32 bit pixel values only on 64-bit systems

    quint32 * pixelData;

#if defined(__x86_64__) || defined(_M_X64)
    quint64 totalPixels = xfCursorImage->height * xfCursorImage->width;
    pixelData = new quint32[totalPixels];

    for (quint64 counter = 0; counter < totalPixels; counter++) {
        pixelData[counter] = xfCursorImage->pixels[counter];
    }
#elif defined(__i386) || defined(_M_IX86)
    pixelData = xfCursorImage->pixels;
#endif

    QImage cursorImage = QImage((quint8*)pixelData, xfCursorImage->width, xfCursorImage->height, QImage::Format_ARGB32_Premultiplied);

    // a small fix for the cursor position for fancier cursors

    cursorPos -= QPoint(xfCursorImage->xhot, xfCursorImage->yhot);

    // now we translate the cursor point to our screen rectangle

    cursorPos -= QPoint(x, y);

    // and do the painting

    QPainter painter(&mPixmap);
    painter.drawImage(cursorPos, cursorImage);

    XFree(xfCursorImage);
#if defined(__x86_64__) || defined(_M_X64)
    delete pixelData;
#endif
}

QPixmap X11ImageGrabber::getWindowPixmap(quint64 winId)
{
    Window window = (Window)winId;
    Display *display = QX11Info::display();
    X11GeometryInfo info;

    XGetGeometry(display, window, &(info.root), &(info.x), &(info.y), &(info.width), &(info.height), &(info.border_width), &(info.depth));
    XImage *ximage = XGetImage(display, window, info.x, info.y, info.width, info.height, AllPlanes, ZPixmap);

    // convert the image into an rgba8888 format. this may not work properly for every
    // possible format of XImage, but 24bpp/32bits seems to be the most common format.

    QPixmap pixmap;

    switch (ximage->depth) {
    case 30:
        pixmap = processXImage30Bit(ximage);
        break;
    case 24:
    case 32:
        pixmap = processXImage32Bit(ximage);
        break;
    default:
        pixmap = QPixmap();
        break;
    }

    // we're done

    XFree(ximage);
    return pixmap;
}

bool X11ImageGrabber::KWinDBusScreenshotAvailable()
{
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered("org.kde.kwin")) {
        QDBusInterface interface("org.kde.kwin", "/Effects", "org.kde.kwin.Effects");
        QDBusReply<bool> reply = interface.call("isEffectLoaded", "screenshot");

        return reply.value();
    }

    return false;
}

void X11ImageGrabber::KWinDBusScreenshotHelper(quint64 winId)
{
    mPixmap = getWindowPixmap(winId);
    emit pixmapChanged(mPixmap);
}

void X11ImageGrabber::KScreenCurrentMonitorScreenshotHelper(KScreen::ConfigOperation *op)
{
    KScreen::ConfigPtr config = qobject_cast<KScreen::GetConfigOperation *>(op)->config();

    if (!config)           { return grabFullScreen(); }
    if (!config->screen()) { return grabFullScreen(); }

    // we'll store the cursor position first

    QPoint cursorPosition = QCursor::pos();

    // next, we'll get all our outputs and figure out which one has the cursor

    const KScreen::OutputList outputs = config->outputs();
    for (auto output: outputs) {
        if (!(output->isConnected())) { continue; }
        if (!(output->currentMode())) { continue; }

        QPoint screenPosition = output->pos();
        QSize  screenSize     = output->currentMode()->size();
        QRect  screenRect     = QRect(screenPosition, screenSize);

        if (!(screenRect.contains(cursorPosition))) {
            continue;
        }

        // bingo, we've found an output that contains the cursor. Now
        // to take a shot

        mPixmap = getWindowPixmap(QX11Info::appRootWindow());
        mPixmap = mPixmap.copy(screenPosition.x(), screenPosition.y(), screenSize.width(), screenSize.height());

        if (mCapturePointer) {
            blendCursorImage(screenPosition.x(), screenPosition.y(), screenSize.width(), screenSize.height());
        }
        emit pixmapChanged(mPixmap);

        co->disconnect();
        co->deleteLater();
        co = nullptr;

        return;
    }

    co->disconnect();
    co->deleteLater();
    co = nullptr;

    return grabFullScreen();
}

// grabber methods

void X11ImageGrabber::grabFullScreen()
{
    Window win = DefaultRootWindow(QX11Info::display());
    mPixmap = getWindowPixmap(win);

    // if we have to blend in the cursor image, do that now

    if (mCapturePointer) {
        XWindowAttributes xwa;
        XGetWindowAttributes(QX11Info::display(), win, &xwa);
        blendCursorImage(xwa.x, xwa.y, xwa.width, xwa.height);
    }

    emit pixmapChanged(mPixmap);
}

void X11ImageGrabber::grabActiveWindow()
{
    Window win = KWindowSystem::activeWindow();

    // if the user doesn't want decorations captured, we're in luck. This is
    // the easiest bit

    if (!mCaptureDecorations) {
        mPixmap = getWindowPixmap(win);

        if (mCapturePointer) {
            X11GeometryInfo info;
            Window child;

            XGetGeometry(QX11Info::display(), win, &(info.root), &(info.x), &(info.y), &(info.width), &(info.height), &(info.border_width), &(info.depth));
            XTranslateCoordinates(QX11Info::display(), win, info.root, 0, 0, &(info.x), &(info.y), &child);

            blendCursorImage(info.x, info.y, info.width, info.height);
        }

        emit pixmapChanged(mPixmap);
        return;
    }

    // This is a KDE app after all, so it's reasonable we find ourselves
    // running under KWin. If we *are* running under KWin, we are in luck,
    // because KWin can take the screenshot for us much more efficiently
    // than we can. It can capture rounded corners, drop shadows, the whole
    // shebang. We'll try to use the kwin dbus interface, if available, to
    // do the job for us

    if (KWinDBusScreenshotAvailable()) {
        QDBusConnection bus = QDBusConnection::sessionBus();
        bus.connect("org.kde.kwin", "/Screenshot", "org.kde.kwin.Screenshot", "screenshotCreated", this, SLOT(KWinDBusScreenshotHelper(quint64)));
        QDBusInterface interface("org.kde.kwin", "/Screenshot", "org.kde.kwin.Screenshot");

        int mask = 1;
        if (mCapturePointer) {
            mask |= 1 << 1;
        }

        interface.call("screenshotForWindow", (quint64)win, mask);
        return;
    }

    // Right, so we're here, which means the KWin screenshot interface is
    // not available to us. So we'll have to drop to native X11 methods.
    // What we're about to do works only for re-parenting window managers.
    // We keep on querying the parent of the window until we can go no
    // further up. That's a handle to the Window Manager frame. But we
    // can't just grab it, because some compositing window managers (yes,
    // KWin included) do not render the window onto the frame but keep it
    // in a seperate OpenGL buffer. So grabbing this window is going to
    // just give us a transparent image with the frame and titlebar.

    // All is not lost. What we need to do is grab the image of the root
    // of the WM frame, find the co-ordinates and geometry of the WM frame
    // in that root, and crop the root image accordingly.

    // I have no clue *why* the next loop works, but it was adapted from the
    // xwininfo code. Copyright notices aren't merged because this is not a
    // "substantial portion"

    while (True) {
        Window root;
        Window parent;
        Window *childList;
        unsigned int nChildren;

        Status status = XQueryTree(QX11Info::display(), win, &root, &parent, &childList, &nChildren);

        if ((parent == root) || !(parent) || !(status)) {
            break;
        }

        if (status && childList) {
            XFree((char *)childList);
        }

        win = parent;
    }

    // now it's just a matter of grabbing...

    X11GeometryInfo info;
    XGetGeometry(QX11Info::display(), win, &(info.root), &(info.x), &(info.y), &(info.width), &(info.height), &(info.border_width), &(info.depth));
    mPixmap = getWindowPixmap(info.root);

    // ...translating co-ordinates...

    Window child;
    XTranslateCoordinates(QX11Info::display(), win, info.root, 0, 0, &(info.x), &(info.y), &child);

    // ...and cropping

    mPixmap = mPixmap.copy(info.x, info.y, info.width, info.height);
    if (mCapturePointer) {
        blendCursorImage(info.x, info.y, info.width, info.height);
    }
    emit pixmapChanged(mPixmap);
}

void X11ImageGrabber::grabCurrentScreen()
{
    co = new KScreen::GetConfigOperation;
    connect(co, &KScreen::GetConfigOperation::finished, this, &X11ImageGrabber::KScreenCurrentMonitorScreenshotHelper);
}
