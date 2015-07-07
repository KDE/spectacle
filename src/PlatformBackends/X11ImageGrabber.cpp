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
{
    mNativeEventFilter = new OnClickEventFilter(this);
}

X11ImageGrabber::~X11ImageGrabber()
{
    delete mNativeEventFilter;
}

// for onClick grab

OnClickEventFilter::OnClickEventFilter(X11ImageGrabber *grabber) :
    QAbstractNativeEventFilter(),
    mImageGrabber(grabber)
{}

bool OnClickEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(result);

    if (eventType == "xcb_generic_event_t") {
        xcb_generic_event_t *ev = static_cast<xcb_generic_event_t *>(message);

        switch (ev->response_type & ~0x80) {
        case XCB_BUTTON_RELEASE:

            // uninstall the eventfilter and release the mouse

            qApp->removeNativeEventFilter(this);
            xcb_ungrab_pointer(QX11Info::connection(), XCB_TIME_CURRENT_TIME);

            // decide whether to grab or abort. regrab the mouse
            // on mouse-wheel events

            {
                xcb_button_release_event_t *ev2 = static_cast<xcb_button_release_event_t *>(message);
                qDebug() << ev2->detail;
                if (ev2->detail == 1) {
                    QMetaObject::invokeMethod(mImageGrabber, "doImageGrab", Qt::QueuedConnection);
                } else if (ev2->detail < 4) {
                    emit mImageGrabber->imageGrabFailed();
                } else {
                    QMetaObject::invokeMethod(mImageGrabber, "doOnClickGrab", Qt::QueuedConnection);
                }
            }
            return true;
        default:
            return false;
        }
    }
    return false;
}

bool X11ImageGrabber::onClickGrabSupported() const
{
    return true;
}

void X11ImageGrabber::doOnClickGrab()
{
    // get the cursor image

    xcb_cursor_t xcbCursor = XCB_CURSOR_NONE;

    xcb_cursor_context_t *xcbCursorCtx;
    xcb_screen_t *xcbAppScreen = xcb_aux_get_screen(QX11Info::connection(), QX11Info::appScreen());

    if (xcb_cursor_context_new(QX11Info::connection(), xcbAppScreen, &xcbCursorCtx) >= 0) {

        QVector<QByteArray> cursorNames = {
            QByteArrayLiteral("cross"),
            QByteArrayLiteral("crosshair"),
            QByteArrayLiteral("diamond-cross"),
            QByteArrayLiteral("cross-reverse")
        };

        for (auto cursorName: cursorNames) {
            xcb_cursor_t cursor = xcb_cursor_load_cursor(xcbCursorCtx, cursorName.constData());
            if (cursor != XCB_CURSOR_NONE) {
                xcbCursor = cursor;
                break;
            }
        }
    }

    // grab the cursor

    xcb_grab_pointer_cookie_t grabPointerCookie = xcb_grab_pointer_unchecked(
        QX11Info::connection(),        // xcb connection
        0,                             // deliver events to owner? nope
        QX11Info::appRootWindow(),     // window to grab pointer for (root)
        XCB_EVENT_MASK_BUTTON_RELEASE, // which events do I want
        XCB_GRAB_MODE_SYNC,            // pointer grab mode
        XCB_GRAB_MODE_ASYNC,           // keyboard grab mode (why is this even here)
        XCB_NONE,                      // confine pointer to which window (none)
        xcbCursor,                     // cursor to change to for the duration of grab
        XCB_TIME_CURRENT_TIME          // do this right now
    );
    CScopedPointer<xcb_grab_pointer_reply_t> grabPointerReply(xcb_grab_pointer_reply(QX11Info::connection(), grabPointerCookie, NULL));

    // if the grab failed, take the screenshot right away

    if (grabPointerReply->status != XCB_GRAB_STATUS_SUCCESS) {
        return doImageGrab();
    }

    // fix things if our pointer grab causes a lockup

    xcb_allow_events(QX11Info::connection(), XCB_ALLOW_SYNC_POINTER, XCB_TIME_CURRENT_TIME);

    // and install our event filter

    qApp->installNativeEventFilter(mNativeEventFilter);

    // done. clean stuff up

    xcb_cursor_context_free(xcbCursorCtx);
    xcb_free_cursor(QX11Info::connection(), xcbCursor);
}

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

QPixmap X11ImageGrabber::blendCursorImage(const QPixmap &pixmap, int x, int y, int width, int height)
{
    // first we get the cursor position, compute the co-ordinates of the region
    // of the screen we're grabbing, and see if the cursor is actually visible in
    // the region

    QPoint cursorPos = QCursor::pos();
    QRect screenRect(x, y, width, height);

    if (!(screenRect.contains(cursorPos))) {
        return pixmap;
    }

    // now we can get the image and start processing

    xcb_connection_t *xcbConn = QX11Info::connection();

    xcb_xfixes_get_cursor_image_cookie_t  cursorCookie = xcb_xfixes_get_cursor_image_unchecked(xcbConn);
    CScopedPointer<xcb_xfixes_get_cursor_image_reply_t>  cursorReply(xcb_xfixes_get_cursor_image_reply(xcbConn, cursorCookie, NULL));
    if (cursorReply.isNull()) {
        return pixmap;
    }

    quint32 *pixelData = xcb_xfixes_get_cursor_image_cursor_image(cursorReply.data());
    if (!pixelData) {
        return pixmap;
    }

    // process the image into a QImage

    QImage cursorImage = QImage((quint8 *)pixelData, cursorReply->width, cursorReply->height, QImage::Format_ARGB32_Premultiplied);

    // a small fix for the cursor position for fancier cursors

    cursorPos -= QPoint(cursorReply->xhot, cursorReply->yhot);

    // now we translate the cursor point to our screen rectangle

    cursorPos -= QPoint(x, y);

    // and do the painting

    QPixmap blendedPixmap = pixmap;
    QPainter painter(&blendedPixmap);
    painter.drawImage(cursorPos, cursorImage);

    // and done

    return blendedPixmap;
}

QPixmap X11ImageGrabber::getWindowPixmap(xcb_window_t window, bool blendPointer)
{
    xcb_connection_t *xcbConn = QX11Info::connection();

    // first get geometry information for our drawable

    xcb_get_geometry_cookie_t geomCookie = xcb_get_geometry_unchecked(xcbConn, window);
    CScopedPointer<xcb_get_geometry_reply_t> geomReply(xcb_get_geometry_reply(xcbConn, geomCookie, NULL));

    // then proceed to get an image

    CScopedPointer<xcb_image_t> xcbImage(
        xcb_image_get(
            xcbConn,
            window,
            geomReply->x,
            geomReply->y,
            geomReply->width,
            geomReply->height,
            ~0,
            XCB_IMAGE_FORMAT_Z_PIXMAP
        )
    );

    // if the image is null, this means we need to get the root image window
    // and run a crop

    if (xcbImage.isNull()) {
        return getWindowPixmap(QX11Info::appRootWindow(), blendPointer)
                .copy(geomReply->x, geomReply->y, geomReply->width, geomReply->height);
    }

    // now process the image

    QPixmap nativePixmap = convertFromNative(xcbImage.data());
    if (!(blendPointer)) {
        return nativePixmap;
    }

    // now we blend in a pointer image

    xcb_get_geometry_cookie_t geomRootCookie = xcb_get_geometry_unchecked(xcbConn, geomReply->root);
    CScopedPointer<xcb_get_geometry_reply_t> geomRootReply(xcb_get_geometry_reply(xcbConn, geomRootCookie, NULL));

    xcb_translate_coordinates_cookie_t translateCookie = xcb_translate_coordinates_unchecked(
        xcbConn, window, geomReply->root, geomRootReply->x, geomRootReply->y);
    CScopedPointer<xcb_translate_coordinates_reply_t> translateReply(
        xcb_translate_coordinates_reply(xcbConn, translateCookie, NULL));

    return blendCursorImage(nativePixmap, translateReply->dst_x,translateReply->dst_y, geomReply->width, geomReply->height);
}

bool X11ImageGrabber::isKWinAvailable()
{
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered("org.kde.KWin")) {
        QDBusInterface interface("org.kde.KWin", "/Effects", "org.kde.kwin.Effects");
        QDBusReply<bool> reply = interface.call("isEffectLoaded", "screenshot");

        return reply.value();
    }

    return false;
}

void X11ImageGrabber::KWinDBusScreenshotHelper(quint64 window)
{
    mPixmap = getWindowPixmap((xcb_window_t)window, false);
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

        mPixmap = getWindowPixmap(QX11Info::appRootWindow(), mCapturePointer);
        mPixmap = mPixmap.copy(screenPosition.x(), screenPosition.y(), screenSize.width(), screenSize.height());
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

void X11ImageGrabber::rectangleSelectionConfirmed(const QPixmap &pixmap, const QRect &region)
{
    QObject *sender = QObject::sender();
    sender->disconnect();
    sender->deleteLater();

    if (mCapturePointer) {
        mPixmap = blendCursorImage(pixmap, region.x(), region.y(), region.width(), region.height());
    } else {
        mPixmap = pixmap;
    }
    emit pixmapChanged(mPixmap);
}

// grabber methods

void X11ImageGrabber::grabFullScreen()
{
    mPixmap = getWindowPixmap(QX11Info::appRootWindow(), mCapturePointer);
    emit pixmapChanged(mPixmap);
}

void X11ImageGrabber::grabTransientWithParent()
{
    xcb_window_t curWin = getRealWindowUnderCursor();

    // do we have a top-level or a transient window?

    KWindowInfo info(curWin, NET::WMName, NET::WM2TransientFor);
    if (!(info.valid(true) && (info.transientFor() != XCB_WINDOW_NONE))) {
        return grabWindowUnderCursor();
    }

    // grab the image early

    mPixmap = getWindowPixmap(QX11Info::appRootWindow(), false);

    // now that we know we have a transient window, let's
    // see if the parent has any other transient windows who's
    // transient for the same app

    QRegion clipRegion;
    QStack<xcb_window_t> childrenStack = findAllChildren(findParent(curWin));

    while (!(childrenStack.isEmpty())) {
        xcb_window_t winId = childrenStack.pop();
        KWindowInfo tempInfo(winId, 0, NET::WM2TransientFor);

        if (info.transientFor() == tempInfo.transientFor()) {
            clipRegion += getApplicationWindowGeometry(winId);
        }
    }

    // now we have a list of all the transient windows for the
    // parent, time to find the parent

    QList<WId> winList = KWindowSystem::stackingOrder();
    for (int i = winList.size() - 1; i >= 0; i--) {
        KWindowInfo tempInfo(winList[i], NET::WMGeometry | NET::WMFrameExtents, NET::WM2WindowClass);

        QString winClass(tempInfo.windowClassClass());
        QString winName(tempInfo.windowClassName());

        if (winClass.contains(info.name(), Qt::CaseInsensitive) || winName.contains(info.name(), Qt::CaseInsensitive)) {
            if (mCaptureDecorations) {
                clipRegion += tempInfo.frameGeometry();
            } else {
                clipRegion += tempInfo.geometry();
            }
            break;
        }
    }

    // we can probably go ahead and generate the image now

    QImage tempImage(mPixmap.size(), QImage::Format_ARGB32);
    tempImage.fill(Qt::transparent);

    QPainter tempPainter(&tempImage);
    tempPainter.setClipRegion(clipRegion);
    tempPainter.drawPixmap(0, 0, mPixmap);
    tempPainter.end();
    mPixmap = QPixmap::fromImage(tempImage).copy(clipRegion.boundingRect());

    // why stop here, when we can render a 20px drop shadow all around it

    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect;
    effect->setOffset(0);
    effect->setBlurRadius(20);

    QGraphicsPixmapItem *item = new QGraphicsPixmapItem;
    item->setPixmap(mPixmap);
    item->setGraphicsEffect(effect);

    QImage shadowImage(mPixmap.size() + QSize(40, 40), QImage::Format_ARGB32);
    shadowImage.fill(Qt::transparent);
    QPainter shadowPainter(&shadowImage);

    QGraphicsScene scene;
    scene.addItem(item);
    scene.render(&shadowPainter, QRectF(), QRectF(-20, -20, mPixmap.width() + 40, mPixmap.height() + 40));
    shadowPainter.end();

    // we can finish up now

    mPixmap = QPixmap::fromImage(shadowImage);
    if (mCapturePointer) {
        QPoint topLeft = clipRegion.boundingRect().topLeft() - QPoint(20, 20);
        mPixmap = blendCursorImage(mPixmap, topLeft.x(), topLeft.y(), mPixmap.width(), mPixmap.height());
    }

    emit pixmapChanged(mPixmap);
}

void X11ImageGrabber::grabActiveWindow()
{
    xcb_window_t activeWindow = KWindowSystem::activeWindow();

    // if KWin is available, use the KWin DBus interfaces

    if (mCaptureDecorations && isKWinAvailable()) {
        QDBusConnection bus = QDBusConnection::sessionBus();
        bus.connect("org.kde.KWin", "/Screenshot", "org.kde.kwin.Screenshot", "screenshotCreated",
                    this, SLOT(KWinDBusScreenshotHelper(quint64)));
        QDBusInterface interface("org.kde.KWin", "/Screenshot", "org.kde.kwin.Screenshot");

        int mask = 1;
        if (mCapturePointer) {
            mask |= 1 << 1;
        }

        interface.call("screenshotForWindow", (quint64)activeWindow, mask);
        return;
    }

    // otherwise, use the native functionality

    return grabApplicationWindowHelper(activeWindow);
}

void X11ImageGrabber::grabWindowUnderCursor()
{
    // if KWin is available, use the KWin DBus interfaces

    if (mCaptureDecorations && isKWinAvailable()) {
        QDBusConnection bus = QDBusConnection::sessionBus();
        bus.connect("org.kde.KWin", "/Screenshot", "org.kde.kwin.Screenshot", "screenshotCreated",
                    this, SLOT(KWinDBusScreenshotHelper(quint64)));
        QDBusInterface interface("org.kde.KWin", "/Screenshot", "org.kde.kwin.Screenshot");

        int mask = 1;
        if (mCapturePointer) {
            mask |= 1 << 1;
        }

        interface.call("screenshotWindowUnderCursor", mask);
        return;
    }

    // else, go native

    return grabApplicationWindowHelper(getRealWindowUnderCursor());
}

void X11ImageGrabber::grabApplicationWindowHelper(xcb_window_t window)
{
    // if the user doesn't want decorations captured, we're in luck. This is
    // the easiest bit

    mPixmap = getWindowPixmap(window, mCapturePointer);
    if (!mCaptureDecorations || window == QX11Info::appRootWindow()) {
        emit pixmapChanged(mPixmap);
        return;
    }

    // if the user wants the window decorations, things get a little tricky.
    // we can't simply get a handle to the window manager frame window and
    // just grab it, because some compositing window managers (yes, kwin
    // included) do not render the window onto the frame but keep it in a
    // separate opengl buffer, so grabbing this window is going to simply
    // give us a transparent image with the frame and titlebar.

    // all is not lost. what we need to do is grab the image of the entire
    // desktop, find the geometry of the window including its frame, and
    // crop the root image accordingly.

    KWindowInfo info(window, NET::WMFrameExtents);
    if (info.valid()) {
        QRect frameGeom = info.frameGeometry();
        mPixmap = getWindowPixmap(QX11Info::appRootWindow(), mCapturePointer).copy(frameGeom);
    }

    // fallback is window without the frame

    emit pixmapChanged(mPixmap);
}

QRect X11ImageGrabber::getApplicationWindowGeometry(xcb_window_t window)
{
    xcb_connection_t *xcbConn = QX11Info::connection();

    xcb_get_geometry_cookie_t geomCookie = xcb_get_geometry_unchecked(xcbConn, window);
    CScopedPointer<xcb_get_geometry_reply_t> geomReply(xcb_get_geometry_reply(xcbConn, geomCookie, NULL));

    return QRect(geomReply->x, geomReply->y, geomReply->width, geomReply->height);
}

void X11ImageGrabber::grabCurrentScreen()
{
    mScreenConfigOperation = new KScreen::GetConfigOperation;
    connect(mScreenConfigOperation, &KScreen::GetConfigOperation::finished,
            this, &X11ImageGrabber::KScreenCurrentMonitorScreenshotHelper);
}

void X11ImageGrabber::grabRectangularRegion()
{
    ScreenClipper *clipper = new ScreenClipper(getWindowPixmap(QX11Info::appRootWindow(), false));

    connect(clipper, &ScreenClipper::regionGrabbed, this, &X11ImageGrabber::rectangleSelectionConfirmed);
    connect(clipper, &ScreenClipper::regionCancelled, this, &X11ImageGrabber::rectangleSelectionCancelled);
}

xcb_window_t X11ImageGrabber::getRealWindowUnderCursor()
{
    xcb_connection_t *xcbConn = QX11Info::connection();
    xcb_window_t curWin = QX11Info::appRootWindow();

    const QByteArray atomName("WM_STATE");
    xcb_intern_atom_cookie_t atomCookie = xcb_intern_atom_unchecked(xcbConn, 0, atomName.length(), atomName.constData());
    xcb_query_pointer_cookie_t pointerCookie = xcb_query_pointer_unchecked(xcbConn, curWin);
    CScopedPointer<xcb_intern_atom_reply_t> atomReply(xcb_intern_atom_reply(xcbConn, atomCookie, NULL));
    CScopedPointer<xcb_query_pointer_reply_t> pointerReply(xcb_query_pointer_reply(xcbConn, pointerCookie, NULL));

    if (atomReply->atom == XCB_ATOM_NONE) {
        return QX11Info::appRootWindow();
    }

    // now start testing

    QStack<xcb_window_t> windowStack;
    windowStack.push(pointerReply->child);

    while (!windowStack.isEmpty()) {
        curWin = windowStack.pop();

        // next, check if our window has the WM_STATE peoperty set on
        // the window. if yes, return the window - we have found it

        xcb_get_property_cookie_t propertyCookie = xcb_get_property_unchecked(xcbConn, 0, curWin, atomReply->atom, XCB_ATOM_ANY, 0, 0);
        CScopedPointer<xcb_get_property_reply_t> propertyReply(xcb_get_property_reply(xcbConn, propertyCookie, NULL));

        if (propertyReply->type != XCB_ATOM_NONE) {
            return curWin;
        }

        // if we're here, this means the window is not the real window
        // we should start looking at its children

        xcb_query_tree_cookie_t treeCookie = xcb_query_tree_unchecked(xcbConn, curWin);
        CScopedPointer<xcb_query_tree_reply_t> treeReply(xcb_query_tree_reply(xcbConn, treeCookie, NULL));
        xcb_window_t *winChildren = xcb_query_tree_children(treeReply.data());
        int winChildrenLength = xcb_query_tree_children_length(treeReply.data());

        for (int i = winChildrenLength - 1; i >= 0; i--) {
            windowStack.push(winChildren[i]);
        }
    }

    // return the window. it has geometry information for a crop

    return pointerReply->child;
}

QStack<xcb_window_t> X11ImageGrabber::findAllChildren(xcb_window_t window)
{
    QStack<xcb_window_t> winStack;
    xcb_connection_t *xcbConn = QX11Info::connection();

    xcb_query_tree_cookie_t treeCookie = xcb_query_tree_unchecked(xcbConn, window);
    CScopedPointer<xcb_query_tree_reply_t> treeReply(xcb_query_tree_reply(xcbConn, treeCookie, NULL));
    xcb_window_t *winChildren = xcb_query_tree_children(treeReply.data());
    int winChildrenLength = xcb_query_tree_children_length(treeReply.data());

    for (int i = winChildrenLength - 1; i >= 0; i--) {
        winStack.push(winChildren[i]);
    }

    return winStack;
}

xcb_window_t X11ImageGrabber::findParent(xcb_window_t window)
{
    xcb_connection_t *xcbConn = QX11Info::connection();

    xcb_query_tree_cookie_t treeCookie = xcb_query_tree_unchecked(xcbConn, window);
    CScopedPointer<xcb_query_tree_reply_t> treeReply(xcb_query_tree_reply(xcbConn, treeCookie, NULL));

    return treeReply->parent;
}
