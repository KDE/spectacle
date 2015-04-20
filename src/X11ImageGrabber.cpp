/*
 *  Copyright (C) 2015 Boudhayan Gupta <me@BaloneyGeek.com>
 *
 *  Contains code from kxutils.cpp, part of KWindowSystem. Copyright
 *  notices reproduced below:
 *
 *  Copyright (C) 2008 Lubos Lunak (l.lunak@kde.org)
 *  Copyright (C) 2013 Martin Gräßlin <mgraesslin@kde.org>
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

#include "X11ImageGrabber.h"

X11ImageGrabber::X11ImageGrabber(QObject *parent) :
    ImageGrabber(parent),
    mScreenConfigOperation(nullptr)
{}

X11ImageGrabber::~X11ImageGrabber()
{}

// image conversion routine

QPixmap X11ImageGrabber::convertFromNative(xcb_image_t *xcbImage)
{
    QImage::Format format = QImage::Format_Invalid;

    switch (xcbImage->depth) {
    case 1:
        format = QImage::Format_MonoLSB;
        break;
    case 16:
        format = QImage::Format_RGB16;
        break;
    case 24:
        format = QImage::Format_RGB32;
        break;
    case 30: {
        // Qt doesn't have a matching image format. We need to convert manually
        quint32 *pixels = reinterpret_cast<quint32 *>(xcbImage->data);
        for (uint i = 0; i < (xcbImage->size / 4); i++) {
            int r = (pixels[i] >> 22) & 0xff;
            int g = (pixels[i] >> 12) & 0xff;
            int b = (pixels[i] >>  2) & 0xff;

            pixels[i] = qRgba(r, g, b, 0xff);
        }
        // fall through, Qt format is still Format_ARGB32_Premultiplied
    }
    case 32:
        format = QImage::Format_ARGB32_Premultiplied;
        break;
    default:
        return QPixmap(); // we don't know
    }

    QImage image(xcbImage->data, xcbImage->width, xcbImage->height, format);

    if (image.isNull()) {
        return QPixmap();
    }

    // work around an abort in QImage::color

    if (image.format() == QImage::Format_MonoLSB) {
        image.setColorCount(2);
        image.setColor(0, QColor(Qt::white).rgb());
        image.setColor(1, QColor(Qt::black).rgb());
    }

    // done

    return QPixmap::fromImage(image);
}

// utility functions

void X11ImageGrabber::blendCursorImage(int x, int y, int width, int height)
{
    // first we get the cursor position, compute the co-ordinates of the region
    // of the screen we're grabbing, and see if the cursor is actually visible in
    // the region

    QPoint cursorPos = QCursor::pos();
    QRect screenRect(x, y, width, height);

    if (!(screenRect.contains(cursorPos))) {
        return;
    }

    // now we can get the image and start processing

    xcb_connection_t *xcbConn = QX11Info::connection();
    xcb_xfixes_get_cursor_image_cookie_t  cursorCookie = xcb_xfixes_get_cursor_image(xcbConn);

    xcb_xfixes_get_cursor_image_reply_t  *cursorReply  = xcb_xfixes_get_cursor_image_reply(xcbConn, cursorCookie, NULL);
    quint32 *pixelData = xcb_xfixes_get_cursor_image_cursor_image(cursorReply);
    QImage cursorImage = QImage((quint8 *)pixelData, cursorReply->width, cursorReply->height, QImage::Format_ARGB32_Premultiplied);

    // a small fix for the cursor position for fancier cursors

    cursorPos -= QPoint(cursorReply->xhot, cursorReply->yhot);

    // now we translate the cursor point to our screen rectangle

    cursorPos -= QPoint(x, y);

    // and do the painting

    QPainter painter(&mPixmap);
    painter.drawImage(cursorPos, cursorImage);

    // and done
}

QPixmap X11ImageGrabber::getWindowPixmap(xcb_window_t window)
{
    xcb_connection_t *xcbConn = QX11Info::connection();

    // first get geometry information for our drawable

    xcb_get_geometry_cookie_t geomCookie = xcb_get_geometry(xcbConn, window);
    xcb_get_geometry_reply_t *geomReply = xcb_get_geometry_reply(xcbConn, geomCookie, NULL);

    // then proceed to get an image

    xcb_image_t *xcbImage = xcb_image_get(xcbConn, window, geomReply->x, geomReply->y, geomReply->width, geomReply->height, ~0, XCB_IMAGE_FORMAT_Z_PIXMAP);

    // now process the image

    return convertFromNative(xcbImage);
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

void X11ImageGrabber::KWinDBusScreenshotHelper(quint64 window)
{
    mPixmap = getWindowPixmap((xcb_window_t)window);
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

        mScreenConfigOperation->disconnect();
        mScreenConfigOperation->deleteLater();
        mScreenConfigOperation = nullptr;

        return;
    }

    mScreenConfigOperation->disconnect();
    mScreenConfigOperation->deleteLater();
    mScreenConfigOperation = nullptr;

    return grabFullScreen();
}

void X11ImageGrabber::rectangleSelectionCancelled()
{
    QObject *sender = QObject::sender();
    sender->disconnect();
    sender->deleteLater();

    emit imageGrabFailed();
}

void X11ImageGrabber::rectangleSelectionConfirmed(int x, int y, int width, int height)
{
    QObject *sender = QObject::sender();
    sender->disconnect();
    sender->deleteLater();

    grabGivenRectangularRegion(x, y, width, height);
}

bool X11ImageGrabber::liveModeAvailable()
{
    // if force non-live mode is set, return false

    if (QProcessEnvironment::systemEnvironment().contains("KSCREENGENIE_FORCE_CROP_NONLIVE")) {
        return false;
    }

    // if compositing is active, live mode is available, and the reverse

    return KWindowSystem::compositingActive();
}

// grabber methods

void X11ImageGrabber::grabFullScreen()
{
    xcb_window_t window = QX11Info::appRootWindow();
    mPixmap = getWindowPixmap(window);

    // if we have to blend in the cursor image, do that now

    if (mCapturePointer) {
        xcb_connection_t *xcbConn = QX11Info::connection();
        xcb_get_geometry_cookie_t geomCookie = xcb_get_geometry(xcbConn, window);
        xcb_get_geometry_reply_t *geomReply = xcb_get_geometry_reply(xcbConn, geomCookie, NULL);

        blendCursorImage(geomReply->x, geomReply->y, geomReply->width, geomReply->height);
    }

    emit pixmapChanged(mPixmap);
}

void X11ImageGrabber::grabActiveWindow()
{
    xcb_window_t window = KWindowSystem::activeWindow();
    xcb_connection_t * xcbConn = QX11Info::connection();

    // if the user doesn't want decorations captured, we're in luck. This is
    // the easiest bit

    if (!mCaptureDecorations) {
        mPixmap = getWindowPixmap(window);

        if (mCapturePointer) {
            xcb_get_geometry_cookie_t geomCookie = xcb_get_geometry(xcbConn, window);
            xcb_get_geometry_reply_t *geomReply = xcb_get_geometry_reply(xcbConn, geomCookie, NULL);

            xcb_get_geometry_cookie_t geomRootCookie = xcb_get_geometry(xcbConn, geomReply->root);
            xcb_get_geometry_reply_t *geomRootReply = xcb_get_geometry_reply(xcbConn, geomRootCookie, NULL);

            xcb_translate_coordinates_cookie_t translateCookie = xcb_translate_coordinates(xcbConn, window, geomReply->root, geomRootReply->x, geomRootReply->y);
            xcb_translate_coordinates_reply_t *translateReply = xcb_translate_coordinates_reply(xcbConn, translateCookie, NULL);

            blendCursorImage(translateReply->dst_x,translateReply->dst_y, geomReply->width, geomReply->height);
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

        interface.call("screenshotForWindow", (quint64)window, mask);
        return;
    }

    // Right, so we're here, which means the KWin screenshot interface is
    // not available to us. So we'll have to drop to native X11 methods.
    // What we're about to do works only for re-parenting window managers.
    // We keep on querying the parent of the window until we can go no
    // further up. That's a handle to the Window Manager frame. But we
    // can't just grab it, because some compositing window managers (yes,
    // KWin included) do not render the window onto the frame but keep it
    // in a separate OpenGL buffer. So grabbing this window is going to
    // just give us a transparent image with the frame and titlebar.

    // All is not lost. What we need to do is grab the image of the root
    // of the WM frame, find the co-ordinates and geometry of the WM frame
    // in that root, and crop the root image accordingly.

    // I have no clue *why* the next loop works, but it was adapted from the
    // xwininfo code. Copyright notices aren't merged because this is not a
    // "substantial portion"

    while (1) {
        xcb_query_tree_cookie_t queryCookie = xcb_query_tree(xcbConn, window);
        xcb_query_tree_reply_t *queryReply = xcb_query_tree_reply(xcbConn, queryCookie, NULL);

        if ((queryReply->parent == queryReply->root) || !(queryReply->parent)) {
            break;
        }

        window = queryReply->parent;
    }

    // now it's just a matter of grabbing...


    xcb_get_geometry_cookie_t geomCookie = xcb_get_geometry(xcbConn, window);
    xcb_get_geometry_reply_t *geomReply = xcb_get_geometry_reply(xcbConn, geomCookie, NULL);
    mPixmap = getWindowPixmap(geomReply->root);

    // ...translating co-ordinates...

    xcb_get_geometry_cookie_t geomRootCookie = xcb_get_geometry(xcbConn, geomReply->root);
    xcb_get_geometry_reply_t *geomRootReply = xcb_get_geometry_reply(xcbConn, geomRootCookie, NULL);

    xcb_translate_coordinates_cookie_t translateCookie = xcb_translate_coordinates(xcbConn, window, geomReply->root, geomRootReply->x, geomRootReply->y);
    xcb_translate_coordinates_reply_t *translateReply = xcb_translate_coordinates_reply(xcbConn, translateCookie, NULL);

    // ...and cropping

    mPixmap = mPixmap.copy(translateReply->dst_x,translateReply->dst_y, geomReply->width, geomReply->height);
    if (mCapturePointer) {
        blendCursorImage(translateReply->dst_x,translateReply->dst_y, geomReply->width, geomReply->height);
    }
    emit pixmapChanged(mPixmap);
}

void X11ImageGrabber::grabCurrentScreen()
{
    mScreenConfigOperation = new KScreen::GetConfigOperation;
    connect(mScreenConfigOperation, &KScreen::GetConfigOperation::finished, this, &X11ImageGrabber::KScreenCurrentMonitorScreenshotHelper);
}

void X11ImageGrabber::grabRectangularRegion()
{
    bool liveMode = liveModeAvailable();
    CropScreenshotGrabber *grabber = new CropScreenshotGrabber(liveMode);

    connect(grabber, &CropScreenshotGrabber::selectionCancelled, this, &X11ImageGrabber::rectangleSelectionCancelled);
    connect(grabber, &CropScreenshotGrabber::selectionConfirmed, this, &X11ImageGrabber::rectangleSelectionConfirmed);

    if (!(liveMode)) {
        mPixmap = getWindowPixmap(QX11Info::appRootWindow());
        grabber->init(mPixmap);
        return;
    }
    grabber->init();
}

void X11ImageGrabber::grabGivenRectangularRegion(int x, int y, int width, int height)
{
    bool liveMode = liveModeAvailable();

    if (liveMode) {
        auto func = [this, x, y, width, height]() mutable { grabGivenRectangularRegionActual(x, y, width, height); };
        QTimer::singleShot(200, func);
        return;
    }

    mPixmap = mPixmap.copy(x, y, width, height);
    emit pixmapChanged(mPixmap);
}

void X11ImageGrabber::grabGivenRectangularRegionActual(int x, int y, int width, int height)
{
    mPixmap = getWindowPixmap(QX11Info::appRootWindow()).copy(x, y, width, height);
    emit pixmapChanged(mPixmap);
}
