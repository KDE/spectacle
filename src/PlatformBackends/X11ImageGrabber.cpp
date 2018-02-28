/*
 *  Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
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

#include <QX11Info>
#include <QStack>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsDropShadowEffect>
#include <QSet>
#include <QtMath>

#include <KWindowSystem>
#include <KWindowInfo>

#include <xcb/xcb_cursor.h>
#include <xcb/xcb_util.h>
#include <xcb/xfixes.h>

#include <X11/Xatom.h>
#include <X11/Xdefs.h>

X11ImageGrabber::X11ImageGrabber(QObject *parent) :
    ImageGrabber(parent)
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

        Q_FOREACH (const QByteArray &cursorName, cursorNames) {
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
    case 30:
        format = QImage::Format_BGR30;
        break;
    case 32:
        format = QImage::Format_ARGB32_Premultiplied;
        break;
    default:
        return QPixmap(); // we don't know
    }

    // The RGB32 format requires data format 0xffRRGGBB, ensure that this fourth byte really is 0xff
    if (format == QImage::Format_RGB32) {
        quint32 *data = reinterpret_cast<quint32 *>(xcbImage->data);
        for (int i = 0; i < xcbImage->width * xcbImage->height; i++) {
            data[i] |= 0xff000000;
        }
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

    // Image is ready. Since the backing data from xcbImage could be freed
    // before the QPixmap goes away, a deep copy is necessary.
    return QPixmap::fromImage(image).copy();
}

// utility functions

// Note: x, y, width and height are measured in device pixels
QPixmap X11ImageGrabber::blendCursorImage(const QPixmap &pixmap, int x, int y, int width, int height)
{
    // If the cursor position lies outside the area, do not bother drawing a cursor.

    QPoint cursorPos = getNativeCursorPosition();
    QRect screenRect(x, y, width, height);

    if (!screenRect.contains(cursorPos)) {
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

QPixmap X11ImageGrabber::getPixmapFromDrawable(xcb_drawable_t drawableId, const QRect &rect)
{
    xcb_connection_t *xcbConn = QX11Info::connection();

    // proceed to get an image based on the geometry (in device pixels)

    QScopedPointer<xcb_image_t, ScopedPointerXcbImageDeleter> xcbImage(
        xcb_image_get(
            xcbConn,
            drawableId,
            rect.x(),
            rect.y(),
            rect.width(),
            rect.height(),
            ~0,
            XCB_IMAGE_FORMAT_Z_PIXMAP
        )
    );

    // too bad, the capture failed.
    if (xcbImage.isNull()) {
        return QPixmap();
    }

    // now process the image

    QPixmap nativePixmap = convertFromNative(xcbImage.data());
    return nativePixmap;
}

// finalize the grabbed pixmap where we know the absolute position
QPixmap X11ImageGrabber::postProcessPixmap(QPixmap pixmap, QRect rect, bool blendPointer)
{
    if (!(blendPointer)) {
        // note: this may be the null pixmap if an error occurred.
        return pixmap;
    }

    return blendCursorImage(pixmap, rect.x(), rect.y(), rect.width(), rect.height());
}

QPixmap X11ImageGrabber::getToplevelPixmap(QRect rect, bool blendPointer)
{
    xcb_window_t rootWindow = QX11Info::appRootWindow();

    // Treat a null rect as an alias for capturing fullscreen
    if (!rect.isValid()) {
        rect = getDrawableGeometry(rootWindow);
    } else {
        QRegion screenRegion;
        for (auto screen : QGuiApplication::screens()) {
            QRect screenRect = screen->geometry();

            // Do not use setSize() here, because QSize::operator*=()
            // performs qRound() which can result in xcb_image_get() failing
            const qreal dpr = screen->devicePixelRatio();
            screenRect.setHeight(qFloor(screenRect.height() * dpr));
            screenRect.setWidth(qFloor(screenRect.width() * dpr));

            screenRegion += screenRect;
        }

        rect = (screenRegion & rect).boundingRect();
    }

    QPixmap nativePixmap = getPixmapFromDrawable(rootWindow, rect);
    return postProcessPixmap(nativePixmap, rect, blendPointer);
}

QPixmap X11ImageGrabber::getWindowPixmap(xcb_window_t window, bool blendPointer)
{
    xcb_connection_t *xcbConn = QX11Info::connection();

    // first get geometry information for our window

    xcb_get_geometry_cookie_t geomCookie = xcb_get_geometry_unchecked(xcbConn, window);
    CScopedPointer<xcb_get_geometry_reply_t> geomReply(xcb_get_geometry_reply(xcbConn, geomCookie, NULL));
    QRect rect(geomReply->x, geomReply->y, geomReply->width, geomReply->height);

    // then proceed to get an image

    QPixmap nativePixmap = getPixmapFromDrawable(window, rect);

    // Translate window coordinates to global ones.

    xcb_get_geometry_cookie_t geomRootCookie = xcb_get_geometry_unchecked(xcbConn, geomReply->root);
    CScopedPointer<xcb_get_geometry_reply_t> geomRootReply(xcb_get_geometry_reply(xcbConn, geomRootCookie, NULL));

    xcb_translate_coordinates_cookie_t translateCookie = xcb_translate_coordinates_unchecked(
        xcbConn, window, geomReply->root, geomRootReply->x, geomRootReply->y);
    CScopedPointer<xcb_translate_coordinates_reply_t> translateReply(
        xcb_translate_coordinates_reply(xcbConn, translateCookie, NULL));

    // Adjust local to global coordinates.
    rect.moveRight(rect.x() + translateReply->dst_x);
    rect.moveTop(rect.y() + translateReply->dst_y);

    // If the window capture failed, try to obtain one from the full screen.
    if (nativePixmap.isNull()) {
        return getToplevelPixmap(rect, blendPointer);
    }

    return postProcessPixmap(nativePixmap, rect, blendPointer);
}

bool X11ImageGrabber::isKWinAvailable()
{
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.KWin"))) {
        QDBusInterface interface(QStringLiteral("org.kde.KWin"), QStringLiteral("/Effects"), QStringLiteral("org.kde.kwin.Effects"));
        QDBusReply<bool> reply = interface.call(QStringLiteral("isEffectLoaded"), QStringLiteral("screenshot"));

        return reply.value();
    }

    return false;
}

void X11ImageGrabber::KWinDBusScreenshotHelper(quint64 pixmapId)
{
    // obtain width and height and grab an image (x and y are always zero for pixmaps)
    QRect rect = getDrawableGeometry((xcb_drawable_t)pixmapId);
    mPixmap = getPixmapFromDrawable((xcb_drawable_t)pixmapId, rect);
    if (!mPixmap.isNull()) {
        emit pixmapChanged(mPixmap);
        return;
    }

    // Cannot retrieve pixmap from KWin, just fallback to fullscreen capture. We
    // could try to detect the original action (window under cursor or active
    // window), but that is too complex for this edge case.
    grabFullScreen();
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

void X11ImageGrabber::updateWindowTitle(xcb_window_t window)
{
    QString windowTitle = KWindowSystem::readNameProperty(window, XA_WM_NAME);
    emit windowTitleChanged(windowTitle);
}

void X11ImageGrabber::grabFullScreen()
{
    mPixmap = getToplevelPixmap(QRect(), mCapturePointer);
    emit pixmapChanged(mPixmap);
}

void X11ImageGrabber::grabTransientWithParent()
{
	xcb_window_t curWin = getRealWindowUnderCursor();
    updateWindowTitle(curWin);

    // grab the image early

    mPixmap = getToplevelPixmap(QRect(), false);

    // now that we know we have a transient window, let's
    // find other possible transient windows and the app window itself.
    QRegion clipRegion;

    QSet<xcb_window_t> transients;
    xcb_window_t parentWinId = curWin;
    const QRect desktopRect(0, 0, 1, 1);
    do {
        // find parent window and add the window to the visible region
        xcb_window_t winId = parentWinId;
        QRect winRect;
        parentWinId = getTransientWindowParent(winId, winRect);
        transients << winId;

        // Don't include the 1x1 pixel sized desktop window in the top left corner that is present
        // if the window is a QDialog without a parent.
        // BUG: 376350
        if (winRect != desktopRect) {
            clipRegion += winRect;
        }

        // Continue walking only if this is a transient window (having a parent)
    } while (parentWinId != XCB_WINDOW_NONE && !transients.contains(parentWinId));


    // All parents are known now, find other transient children.
    // Assume that the lowest window is behind everything else, then if a new
    // transient window is discovered, its children can then also be found.

    QList<WId> winList = KWindowSystem::stackingOrder();
    for (auto winId : winList) {
        QRect winRect;
        xcb_window_t parentWinId = getTransientWindowParent(winId, winRect);

        // if the parent should be displayed, then show the child too
        if (transients.contains(parentWinId)) {
            if (!transients.contains(winId)) {
                transients << winId;
                clipRegion += winRect;
            }
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
    updateWindowTitle(activeWindow);

    // if KWin is available, use the KWin DBus interfaces

    if (mCaptureDecorations && isKWinAvailable()) {
        QDBusConnection bus = QDBusConnection::sessionBus();
        bus.connect(QStringLiteral("org.kde.KWin"),
                    QStringLiteral("/Screenshot"),
                    QStringLiteral("org.kde.kwin.Screenshot"),
                    QStringLiteral("screenshotCreated"),
                    this, SLOT(KWinDBusScreenshotHelper(quint64)));
        QDBusInterface interface(QStringLiteral("org.kde.KWin"), QStringLiteral("/Screenshot"), QStringLiteral("org.kde.kwin.Screenshot"));

        int mask = 1;
        if (mCapturePointer) {
            mask |= 1 << 1;
        }

        interface.call(QStringLiteral("screenshotForWindow"), (quint64)activeWindow, mask);
        return;
    }

    // otherwise, use the native functionality

    return grabApplicationWindowHelper(activeWindow);
}

void X11ImageGrabber::grabWindowUnderCursor()
{
    const xcb_window_t windowUnderCursor = getRealWindowUnderCursor();
    updateWindowTitle(windowUnderCursor);

    // if KWin is available, use the KWin DBus interfaces

    if (mCaptureDecorations && isKWinAvailable()) {
        QDBusConnection bus = QDBusConnection::sessionBus();
        bus.connect(QStringLiteral("org.kde.KWin"),
                    QStringLiteral("/Screenshot"),
                    QStringLiteral("org.kde.kwin.Screenshot"),
                    QStringLiteral("screenshotCreated"),
                    this, SLOT(KWinDBusScreenshotHelper(quint64)));
        QDBusInterface interface(QStringLiteral("org.kde.KWin"), QStringLiteral("/Screenshot"), QStringLiteral("org.kde.kwin.Screenshot"));

        int mask = 1;
        if (mCapturePointer) {
            mask |= 1 << 1;
        }

        interface.call(QStringLiteral("screenshotWindowUnderCursor"), mask);
        return;
    }

    // else, go native

    return grabApplicationWindowHelper(windowUnderCursor);
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
        mPixmap = getToplevelPixmap(frameGeom, mCapturePointer);
    }

    // fallback is window without the frame

    emit pixmapChanged(mPixmap);
}

QRect X11ImageGrabber::getDrawableGeometry(xcb_drawable_t drawable)
{
    xcb_connection_t *xcbConn = QX11Info::connection();

    xcb_get_geometry_cookie_t geomCookie = xcb_get_geometry_unchecked(xcbConn, drawable);
    CScopedPointer<xcb_get_geometry_reply_t> geomReply(xcb_get_geometry_reply(xcbConn, geomCookie, NULL));
    if (geomReply.isNull()) {
        return QRect();
    }
    return QRect(geomReply->x, geomReply->y, geomReply->width, geomReply->height);
}

void X11ImageGrabber::grabCurrentScreen()
{
    QPoint cursorPosition = QCursor::pos();
    for (auto screen : QGuiApplication::screens()) {
        QRect screenRect = screen->geometry();
        if (!screenRect.contains(cursorPosition)) {
            continue;
        }

        // The screen origin is in native pixels, but the size is device-dependent. Convert these also to native pixels.
        QRect nativeScreenRect(screenRect.topLeft(), screenRect.size() * screen->devicePixelRatio());
        mPixmap = getToplevelPixmap(nativeScreenRect, mCapturePointer);
        emit pixmapChanged(mPixmap);
        return;
    }

    // No screen found with our cursor, fallback to capturing full screen
    grabFullScreen();
}

void X11ImageGrabber::grabRectangularRegion()
{
    QuickEditor *editor = new QuickEditor(getToplevelPixmap(QRect(), false));

    connect(editor, &QuickEditor::grabDone, this, &X11ImageGrabber::rectangleSelectionConfirmed);
    connect(editor, &QuickEditor::grabCancelled, this, &X11ImageGrabber::rectangleSelectionCancelled);
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


// obtain the size of the given window, returning the window ID of the parent
xcb_window_t X11ImageGrabber::getTransientWindowParent(xcb_window_t winId, QRect &outRect)
{
    NET::Properties properties = mCaptureDecorations ? NET::WMFrameExtents : NET::WMGeometry;
    KWindowInfo winInfo(winId, properties, NET::WM2TransientFor);

    // add the current window to the image
    if (mCaptureDecorations) {
        outRect = winInfo.frameGeometry();
    } else {
        outRect = winInfo.geometry();
    }
    return winInfo.transientFor();
}

QPoint X11ImageGrabber::getNativeCursorPosition()
{
    // QCursor::pos() is not used because it requires additional calculations.
    // Its value is the offset to the origin of the current screen is in
    // device-independent pixels while the origin itself uses native pixels.

    xcb_connection_t *xcbConn = QX11Info::connection();
    xcb_query_pointer_cookie_t pointerCookie = xcb_query_pointer_unchecked(xcbConn, QX11Info::appRootWindow());
    CScopedPointer<xcb_query_pointer_reply_t> pointerReply(xcb_query_pointer_reply(xcbConn, pointerCookie, NULL));

    return QPoint(pointerReply->root_x, pointerReply->root_y);
}
