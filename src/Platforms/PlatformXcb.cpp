/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PlatformXcb.h"

#include <xcb/xcb_cursor.h>
#include <xcb/xcb_util.h>
#include <xcb/xfixes.h>

#include <X11/Xatom.h>
#include <X11/Xdefs.h>

#include <QAbstractNativeEventFilter>
#include <QApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QPainter>
#include <QScreen>
#include <QSet>
#include <QStack>
#include <QTimer>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QX11Info>
#else
#include <private/qtx11extras_p.h>
#endif
#include <QtMath>

#include <KWindowInfo>
#include <KWindowSystem>
#include <KX11Extras>

#include <memory>

/* -- XCB Image Smart Pointer ------------------------------------------------------------------ */

struct XcbImagePtrDeleter {
    void operator()(xcb_image_t *xcbImage) const
    {
        if (xcbImage) {
            xcb_image_destroy(xcbImage);
        }
    }
};
using XcbImagePtr = std::unique_ptr<xcb_image_t, XcbImagePtrDeleter>;

struct CFreeDeleter {
    void operator()(void *ptr) const
    {
        free(ptr);
    }
};
template<typename Reply>
using XcbReplyPtr = std::unique_ptr<Reply, CFreeDeleter>;

/* -- On Click Native Event Filter ------------------------------------------------------------- */

class PlatformXcb::OnClickEventFilter : public QAbstractNativeEventFilter
{
public:
    explicit OnClickEventFilter(PlatformXcb *platformPtr)
        : m_platformPtr(platformPtr)
    {
    }

    void setCaptureOptions(Platform::GrabMode grabMode, bool includePointer, bool includeDecorations)
    {
        m_grabMode = grabMode;
        m_includePointer = includePointer;
        m_includeDecorations = includeDecorations;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    bool nativeEventFilter(const QByteArray &eventType, void *message, long * /* result */) override
#else
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr * /*result*/) override
#endif
    {
        if (eventType == "xcb_generic_event_t") {
            auto firstEvent = static_cast<xcb_generic_event_t *>(message);

            switch (firstEvent->response_type & ~0x80) {
            case XCB_BUTTON_RELEASE: {
                // uninstall the eventfilter and release the mouse
                qApp->removeNativeEventFilter(this);
                xcb_ungrab_pointer(QX11Info::connection(), XCB_TIME_CURRENT_TIME);

                // decide whether to grab or abort. regrab the mouse on mouse-wheel events
                {
                    auto secondEvent = static_cast<xcb_button_release_event_t *>(message);
                    if (secondEvent->detail == 1) {
                        QTimer::singleShot(0, nullptr, [this]() {
                            m_platformPtr->doGrabNow(m_grabMode, m_includePointer, m_includeDecorations);
                        });
                    } else if (secondEvent->detail == 2 || secondEvent->detail == 3) {
                        // 2: middle click, 3: right click; both cancel
                        Q_EMIT m_platformPtr->newScreenshotTaken();
                    } else if (secondEvent->detail < 4) {
                        Q_EMIT m_platformPtr->newScreenshotFailed();
                    } else {
                        QTimer::singleShot(0, nullptr, [this]() {
                            m_platformPtr->doGrabOnClick(m_grabMode, m_includePointer, m_includeDecorations);
                        });
                    }
                }
                return true;
            }
            default:
                return false;
            }
        }
        return false;
    }

private:
    PlatformXcb *m_platformPtr;
    Platform::GrabMode m_grabMode{GrabMode::AllScreens};
    bool m_includePointer{true};
    bool m_includeDecorations{true};
};

/* -- General Plumbing ------------------------------------------------------------------------- */

PlatformXcb::PlatformXcb(QObject *parent)
    : Platform(parent)
    , m_nativeEventFilter(new OnClickEventFilter(this))
{
    updateSupportedGrabModes();
    connect(qGuiApp, &QGuiApplication::screenAdded, this, &PlatformXcb::updateSupportedGrabModes);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &PlatformXcb::updateSupportedGrabModes);
}

PlatformXcb::~PlatformXcb()
{
}

Platform::GrabModes PlatformXcb::supportedGrabModes() const
{
    return m_grabModes;
}

void PlatformXcb::updateSupportedGrabModes()
{
    Platform::GrabModes grabModes = {
        GrabMode::AllScreens, GrabMode::ActiveWindow,
        GrabMode::WindowUnderCursor, GrabMode::TransientWithParent,
        GrabMode::PerScreenImageNative
    };

    if (QApplication::screens().count() > 1) {
        grabModes |= Platform::GrabMode::CurrentScreen;
    }

    if (m_grabModes != grabModes) {
        m_grabModes = grabModes;
        Q_EMIT supportedGrabModesChanged();
    }
}

Platform::ShutterModes PlatformXcb::supportedShutterModes() const
{
    return {ShutterMode::Immediate | ShutterMode::OnClick};
}

void PlatformXcb::doGrab(ShutterMode shutterMode, GrabMode grabMode, bool includePointer, bool includeDecorations)
{
    switch (shutterMode) {
    case ShutterMode::Immediate: {
        doGrabNow(grabMode, includePointer, includeDecorations);
        return;
    }
    case ShutterMode::OnClick: {
        doGrabOnClick(grabMode, includePointer, includeDecorations);
        return;
    }
    }
}

/* -- Platform Utilities ----------------------------------------------------------------------- */

void PlatformXcb::updateWindowTitle(xcb_window_t window)
{
    auto title = KX11Extras::readNameProperty(window, XA_WM_NAME);
    Q_EMIT windowTitleChanged(title);
}

bool PlatformXcb::isKWinAvailable()
{
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.KWin"))) {
        QDBusInterface iface(QStringLiteral("org.kde.KWin"), QStringLiteral("/Effects"), QStringLiteral("org.kde.kwin.Effects"));
        QDBusReply<bool> reply = iface.call(QStringLiteral("isEffectLoaded"), QStringLiteral("screenshot"));
        return reply.value();
    }
    return false;
}

/* -- XCB Utilities ---------------------------------------------------------------------------- */

QPoint PlatformXcb::getCursorPosition()
{
    // QCursor::pos() is not used because it requires additional calculations.
    // Its value is the offset to the origin of the current screen is in
    // device-independent pixels while the origin itself uses native pixels.

    auto xcbConn = QX11Info::connection();
    auto pointerCookie = xcb_query_pointer_unchecked(xcbConn, QX11Info::appRootWindow());
    XcbReplyPtr<xcb_query_pointer_reply_t> pointerReply(xcb_query_pointer_reply(xcbConn, pointerCookie, nullptr));

    return QPoint(pointerReply->root_x, pointerReply->root_y);
}

QRect PlatformXcb::getDrawableGeometry(xcb_drawable_t drawable)
{
    auto xcbConn = QX11Info::connection();
    auto geoCookie = xcb_get_geometry_unchecked(xcbConn, drawable);
    XcbReplyPtr<xcb_get_geometry_reply_t> geoReply(xcb_get_geometry_reply(xcbConn, geoCookie, nullptr));
    if (!geoReply) {
        return QRect();
    }
    return QRect(geoReply->x, geoReply->y, geoReply->width, geoReply->height);
}

xcb_window_t PlatformXcb::getWindowUnderCursor()
{
    auto xcbConn = QX11Info::connection();
    auto appWin = QX11Info::appRootWindow();

    const QByteArray atomName("WM_STATE");
    auto atomCookie = xcb_intern_atom_unchecked(xcbConn, 0, atomName.length(), atomName.constData());
    auto pointerCookie = xcb_query_pointer_unchecked(xcbConn, appWin);
    XcbReplyPtr<xcb_intern_atom_reply_t> atomReply(xcb_intern_atom_reply(xcbConn, atomCookie, nullptr));
    XcbReplyPtr<xcb_query_pointer_reply_t> pointerReply(xcb_query_pointer_reply(xcbConn, pointerCookie, nullptr));

    if (atomReply->atom == XCB_ATOM_NONE) {
        return QX11Info::appRootWindow();
    }

    // now start testing
    QStack<xcb_window_t> windowStack;
    windowStack.push(pointerReply->child);

    while (!windowStack.isEmpty()) {
        appWin = windowStack.pop();

        // next, check if our window has the WM_STATE property set on
        // the window. if yes, return the window - we have found it
        auto propCookie = xcb_get_property_unchecked(xcbConn, 0, appWin, atomReply->atom, XCB_ATOM_ANY, 0, 0);
        XcbReplyPtr<xcb_get_property_reply_t> propReply(xcb_get_property_reply(xcbConn, propCookie, nullptr));

        if (propReply->type != XCB_ATOM_NONE) {
            return appWin;
        }

        // if we're here, this means the window is not the real window
        // we should start looking at its children
        auto treeCookie = xcb_query_tree_unchecked(xcbConn, appWin);
        XcbReplyPtr<xcb_query_tree_reply_t> treeReply(xcb_query_tree_reply(xcbConn, treeCookie, nullptr));
        auto windowChildren = xcb_query_tree_children(treeReply.get());
        auto windowChildrenLength = xcb_query_tree_children_length(treeReply.get());

        for (int iIdx = windowChildrenLength - 1; iIdx >= 0; iIdx--) {
            windowStack.push(windowChildren[iIdx]);
        }
    }

    // return the window. it has geometry information for a crop
    return pointerReply->child;
}

xcb_window_t PlatformXcb::getTransientWindowParent(xcb_window_t childWindow, QRect &windowRectOut, bool includeDecorations)
{
    NET::Properties netProp = includeDecorations ? NET::WMFrameExtents : NET::WMGeometry;
    KWindowInfo windowInfo(childWindow, netProp, NET::WM2TransientFor);

    // add the current window to the image
    if (includeDecorations) {
        windowRectOut = windowInfo.frameGeometry();
    } else {
        windowRectOut = windowInfo.geometry();
    }
    return windowInfo.transientFor();
}

/* -- Image Processing Utilities --------------------------------------------------------------- */

QImage PlatformXcb::convertFromNative(xcb_image_t *xcbImage)
{
    auto imageFormat = QImage::Format_Invalid;
    switch (xcbImage->depth) {
    case 1:
        imageFormat = QImage::Format_MonoLSB;
        break;
    case 16:
        imageFormat = QImage::Format_RGB16;
        break;
    case 24:
        imageFormat = QImage::Format_RGB32;
        break;
    case 30:
        imageFormat = QImage::Format_RGB30;
        break;
    case 32:
        imageFormat = QImage::Format_ARGB32_Premultiplied;
        break;
    default:
        return {}; // we don't know
    }

    // the RGB32 format requires data format 0xffRRGGBB, ensure that this fourth byte really is 0xff
    if (imageFormat == QImage::Format_RGB32) {
        auto data = reinterpret_cast<quint32 *>(xcbImage->data);
        for (size_t iIter = 0; iIter < xcbImage->width * xcbImage->height; iIter++) {
            data[iIter] |= 0xff000000;
        }
    }

    QImage image(xcbImage->data, xcbImage->width, xcbImage->height, imageFormat);
    if (image.isNull()) {
        return {};
    }

    // work around an abort in QImage::color
    if (image.format() == QImage::Format_MonoLSB) {
        image.setColorCount(2);
        image.setColor(0, QColor(Qt::white).rgb());
        image.setColor(1, QColor(Qt::black).rgb());
    }

    // the image is ready. Since the backing data from xcbImage could be freed
    // before the image goes away, a deep copy is necessary.
    return image.copy();
}

QImage PlatformXcb::blendCursorImage(QImage &image, const QRect rect)
{
    // If the cursor position lies outside the area, do not bother drawing a cursor.

    auto cursorPos = getCursorPosition();
    if (!rect.contains(cursorPos)) {
        return image;
    }

    // now we can get the image and start processing
    auto xcbConn = QX11Info::connection();

    auto cursorCookie = xcb_xfixes_get_cursor_image_unchecked(xcbConn);
    XcbReplyPtr<xcb_xfixes_get_cursor_image_reply_t> cursorReply(xcb_xfixes_get_cursor_image_reply(xcbConn, cursorCookie, nullptr));
    if (!cursorReply) {
        return image;
    }

    // get the image and process it into a qimage
    auto pixelData = xcb_xfixes_get_cursor_image_cursor_image(cursorReply.get());
    if (!pixelData) {
        return image;
    }
    QImage cursorImage(reinterpret_cast<quint8 *>(pixelData), cursorReply->width, cursorReply->height, QImage::Format_ARGB32_Premultiplied);

    // a small fix for the cursor position for fancier cursors
    cursorPos -= QPoint(cursorReply->xhot, cursorReply->yhot);

    // now we translate the cursor point to our screen rectangle and do the painting
    cursorPos -= QPoint(rect.x(), rect.y());
    QPainter painter(&image);
    painter.drawImage(cursorPos, cursorImage);
    return image;
}

QImage PlatformXcb::postProcessImage(QImage &image, QRect rect, bool blendPointer)
{
    if (!(blendPointer)) {
        // note: this may be a null image if an error occurred.
        return image;
    }
    return blendCursorImage(image, rect);
}

/* -- Capture Helpers -------------------------------------------------------------------------- */

QImage PlatformXcb::getImageFromDrawable(xcb_drawable_t xcbDrawable, const QRect &rect)
{
    auto xcbConn = QX11Info::connection();

    // proceed to get an image based on the geometry (in device pixels)
    XcbImagePtr xcbImage(xcb_image_get(xcbConn, xcbDrawable, rect.x(), rect.y(), rect.width(), rect.height(), ~0, XCB_IMAGE_FORMAT_Z_PIXMAP));

    // too bad, the capture failed.
    if (!xcbImage) {
        return {};
    }

    // now process the image
    return convertFromNative(xcbImage.get());
}

QImage PlatformXcb::getToplevelImage(QRect rect, bool blendPointer)
{
    auto rootWindow = QX11Info::appRootWindow();

    // treat a null rect as an alias for capturing fullscreen
    if (!rect.isValid()) {
        rect = getDrawableGeometry(rootWindow);
    } else {
        QRegion screenRegion;
        const auto screens = QGuiApplication::screens();
        for (auto screen : screens) {
            auto screenRect = screen->geometry();

            // Do not use setSize() here, because QSize::operator*=()
            // performs qRound() which can result in xcb_image_get() failing
            const auto pixelRatio = screen->devicePixelRatio();
            screenRect.setHeight(qFloor(screenRect.height() * pixelRatio));
            screenRect.setWidth(qFloor(screenRect.width() * pixelRatio));

            screenRegion += screenRect;
        }
        rect = (screenRegion & rect).boundingRect();
    }

    auto image = getImageFromDrawable(rootWindow, rect);
    return postProcessImage(image, rect, blendPointer);
}

QImage PlatformXcb::getWindowImage(xcb_window_t window, bool blendPointer)
{
    auto xcbConn = QX11Info::connection();

    // first get geometry information for our window
    auto geoCookie = xcb_get_geometry_unchecked(xcbConn, window);
    XcbReplyPtr<xcb_get_geometry_reply_t> geoReply(xcb_get_geometry_reply(xcbConn, geoCookie, nullptr));
    QRect windowRect(geoReply->x, geoReply->y, geoReply->width, geoReply->height);

    // then proceed to get an image
    auto image = getImageFromDrawable(window, windowRect);

    // translate window coordinates to global ones.
    auto rootGeoCookie = xcb_get_geometry_unchecked(xcbConn, geoReply->root);
    XcbReplyPtr<xcb_get_geometry_reply_t> rootGeoReply(xcb_get_geometry_reply(xcbConn, rootGeoCookie, nullptr));
    auto translateCookie = xcb_translate_coordinates_unchecked(xcbConn, window, geoReply->root, rootGeoReply->x, rootGeoReply->y);
    XcbReplyPtr<xcb_translate_coordinates_reply_t> translateReply(xcb_translate_coordinates_reply(xcbConn, translateCookie, nullptr));

    // adjust local to global coordinates.
    windowRect.moveRight(windowRect.x() + translateReply->dst_x);
    windowRect.moveTop(windowRect.y() + translateReply->dst_y);

    // if the window capture failed, try to obtain one from the full screen.
    if (image.isNull()) {
        return getToplevelImage(windowRect, blendPointer);
    }
    return postProcessImage(image, windowRect, blendPointer);
}

void PlatformXcb::handleKWinScreenshotReply(quint64 drawable)
{
    QDBusConnection::sessionBus().disconnect(QStringLiteral("org.kde.KWin"),
                                             QStringLiteral("/Screenshot"),
                                             QStringLiteral("org.kde.kwin.Screenshot"),
                                             QStringLiteral("screenshotCreated"),
                                             this,
                                             SLOT(handleKWinScreenshotReply(quint64)));

    // obtain width and height and grab an image (x and y are always zero for drawables)
    auto xcbDrawable = static_cast<xcb_drawable_t>(drawable);
    auto rect = getDrawableGeometry(xcbDrawable);
    auto image = getImageFromDrawable(xcbDrawable, rect);

    if (!image.isNull()) {
        image.setDevicePixelRatio(qGuiApp->devicePixelRatio());
        Q_EMIT newScreenshotTaken(image);
        return;
    }
    Q_EMIT newScreenshotFailed();
}

/* -- Grabber Methods -------------------------------------------------------------------------- */

void PlatformXcb::grabAllScreens(bool includePointer)
{
    auto image = getToplevelImage(QRect(), includePointer);
    image.setDevicePixelRatio(qGuiApp->devicePixelRatio());
    Q_EMIT newScreenshotTaken(image);
}

void PlatformXcb::grabCurrentScreen(bool includePointer)
{
    auto cursorPosition = QCursor::pos();
    const auto screens = QGuiApplication::screens();
    for (auto screen : screens) {
        auto screenRect = screen->geometry();
        if (!screenRect.contains(cursorPosition)) {
            continue;
        }

        // the screen origin is in native pixels, but the size is device-dependent.
        // convert these also to native pixels.
        QRect nativeScreenRect(screenRect.topLeft(), screenRect.size() * screen->devicePixelRatio());
        auto image = getToplevelImage(nativeScreenRect, includePointer);
        image.setDevicePixelRatio(qGuiApp->devicePixelRatio());
        Q_EMIT newScreenshotTaken(image);
        return;
    }

    // no screen found with our cursor, fallback to capturing all screens
    grabAllScreens(includePointer);
}

void PlatformXcb::grabApplicationWindow(xcb_window_t window, bool includePointer, bool includeDecorations)
{
    // if the user doesn't want decorations captured, we're in luck. This is
    // the easiest bit

    auto image = getWindowImage(window, includePointer);
    image.setDevicePixelRatio(qGuiApp->devicePixelRatio());
    if (!includeDecorations || window == QX11Info::appRootWindow()) {
        Q_EMIT newScreenshotTaken(image);
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

    KWindowInfo windowInfo(window, NET::WMFrameExtents);
    if (windowInfo.valid()) {
        auto frameGeom = windowInfo.frameGeometry();
        image = getToplevelImage(frameGeom, includePointer);
    }

    // fallback is window without the frame
    Q_EMIT newScreenshotTaken(image);
}

void PlatformXcb::grabActiveWindow(bool includePointer, bool includeDecorations)
{
    auto activeWindow = KX11Extras::activeWindow();
    updateWindowTitle(activeWindow);

    // if KWin is available, use the KWin DBus interfaces
    if (includeDecorations && isKWinAvailable()) {
        auto bus = QDBusConnection::sessionBus();
        bus.connect(QStringLiteral("org.kde.KWin"),
                     QStringLiteral("/Screenshot"),
                     QStringLiteral("org.kde.kwin.Screenshot"),
                     QStringLiteral("screenshotCreated"),
                     this,
                     SLOT(handleKWinScreenshotReply(quint64)));
        QDBusInterface iface(QStringLiteral("org.kde.KWin"), QStringLiteral("/Screenshot"), QStringLiteral("org.kde.kwin.Screenshot"));

        int opMask = 1;
        if (includePointer) {
            opMask |= 1 << 1;
        }
        iface.call(QStringLiteral("screenshotForWindow"), static_cast<quint64>(activeWindow), opMask);

        return;
    }

    // otherwise, use the native functionality
    grabApplicationWindow(activeWindow, includePointer, includeDecorations);
}

void PlatformXcb::grabWindowUnderCursor(bool includePointer, bool includeDecorations)
{
    auto window = getWindowUnderCursor();
    updateWindowTitle(window);

    // if KWin is available, use the KWin DBus interfaces
    if (includeDecorations && isKWinAvailable()) {
        auto bus = QDBusConnection::sessionBus();
        bus.connect(QStringLiteral("org.kde.KWin"),
                     QStringLiteral("/Screenshot"),
                     QStringLiteral("org.kde.kwin.Screenshot"),
                     QStringLiteral("screenshotCreated"),
                     this,
                     SLOT(handleKWinScreenshotReply(quint64)));
        QDBusInterface interface(QStringLiteral("org.kde.KWin"), QStringLiteral("/Screenshot"), QStringLiteral("org.kde.kwin.Screenshot"));

        int opMask = 1;
        if (includePointer) {
            opMask |= 1 << 1;
        }
        interface.call(QStringLiteral("screenshotWindowUnderCursor"), opMask);

        return;
    }

    // otherwise, use the native functionality
    grabApplicationWindow(window, includePointer, includeDecorations);
}

void PlatformXcb::grabTransientWithParent(bool includePointer, bool includeDecorations)
{
    auto window = getWindowUnderCursor();
    updateWindowTitle(window);

    // grab the image early
    auto image = getToplevelImage(QRect(), false);
    image.setDevicePixelRatio(qGuiApp->devicePixelRatio());

    // now that we know we have a transient window, let's
    // find other possible transient windows and the app window itself.
    QRegion clipRegion;
    QSet<xcb_window_t> transientWindows;
    auto parentWindow = window;
    const QRect desktopRect(0, 0, 1, 1);
    do {
        // find parent window and add the window to the visible region
        auto winId = parentWindow;
        QRect winRect;
        parentWindow = getTransientWindowParent(winId, winRect, includeDecorations);
        transientWindows << winId;

        // Don't include the 1x1 pixel sized desktop window in the top left corner that is present
        // if the window is a QDialog without a parent.
        // BUG: 376350
        if (winRect != desktopRect) {
            clipRegion += winRect;
        }

        // Continue walking only if this is a transient window (having a parent)
    } while (parentWindow != XCB_WINDOW_NONE && !transientWindows.contains(parentWindow));

    // All parents are known now, find other transient children.
    // Assume that the lowest window is behind everything else, then if a new
    // transient window is discovered, its children can then also be found.
    auto winList = KX11Extras::stackingOrder();
    for (auto winId : winList) {
        QRect winRect;
        auto parentWindow = getTransientWindowParent(winId, winRect, includeDecorations);

        // if the parent should be displayed, then show the child too
        if (transientWindows.contains(parentWindow)) {
            if (!transientWindows.contains(winId)) {
                transientWindows << winId;
                clipRegion += winRect;
            }
        }
    }

    // we can probably go ahead and generate the image now
    QImage clippedImage(image.size(), QImage::Format_ARGB32);
    clippedImage.fill(Qt::transparent);

    QPainter painter(&clippedImage);
    painter.setClipRegion(clipRegion);
    painter.drawImage(0, 0, image);
    painter.end();
    image = clippedImage.copy(clipRegion.boundingRect());

    // why stop here, when we can render a 20px drop shadow all around it
    auto shadowEffect = new QGraphicsDropShadowEffect;
    shadowEffect->setOffset(0);
    shadowEffect->setBlurRadius(20);

    auto pixmapItem = new QGraphicsPixmapItem;
    pixmapItem->setPixmap(QPixmap::fromImage(image));
    pixmapItem->setGraphicsEffect(shadowEffect);

    QImage shadowImage(image.size() + QSize(40, 40), QImage::Format_ARGB32);
    shadowImage.fill(Qt::transparent);
    QPainter shadowPainter(&shadowImage);

    QGraphicsScene graphicsScene;
    graphicsScene.addItem(pixmapItem);
    graphicsScene.render(&shadowPainter, QRectF(), QRectF(-20, -20, image.width() + 40, image.height() + 40));
    shadowPainter.end();

    // we can finish up now
    image = shadowImage;
    if (includePointer) {
        auto topLeft = clipRegion.boundingRect().topLeft() - QPoint(20, 20);
        image = blendCursorImage(image, QRect(topLeft, QSize(image.width(), image.height())));
    }

    Q_EMIT newScreenshotTaken(image);
}

void PlatformXcb::doGrabNow(GrabMode grabMode, bool includePointer, bool includeDecorations)
{
    if (grabMode & ~(ActiveWindow | WindowUnderCursor | TransientWithParent)) {
        // Notify that window title is empty since we are not picking a window.
        Q_EMIT windowTitleChanged();
    }
    switch (grabMode) {
    case GrabMode::AllScreens:
    case GrabMode::AllScreensScaled:
        grabAllScreens(includePointer);
        break;
    case GrabMode::PerScreenImageNative: {
        auto image = getToplevelImage(QRect(), includePointer);
        // break the image into a list of images
        const auto screens = QGuiApplication::screens();
        QVector<CanvasImage> screenImages;
        for (const auto screen : screens) {
            QRect imageRect = screen->geometry();
            imageRect.setSize(screen->size() * screen->devicePixelRatio());
            screenImages.append({image.copy(imageRect), screen->geometry()});
        }
        Q_EMIT newScreensScreenshotTaken(screenImages);
        break;
    }
    case GrabMode::CurrentScreen:
        grabCurrentScreen(includePointer);
        break;
    case GrabMode::ActiveWindow:
        grabActiveWindow(includePointer, includeDecorations);
        break;
    case GrabMode::WindowUnderCursor:
        grabWindowUnderCursor(includePointer, includeDecorations);
        break;
    case GrabMode::TransientWithParent:
        grabTransientWithParent(includePointer, includeDecorations);
        break;
    case GrabMode::NoGrabModes:
        Q_EMIT newScreenshotFailed();
    }
}

void PlatformXcb::doGrabOnClick(GrabMode grabMode, bool includePointer, bool includeDecorations)
{
    // get the cursor image
    xcb_cursor_t xcbCursor = XCB_CURSOR_NONE;
    xcb_cursor_context_t *xcbCursorCtx = nullptr;
    xcb_screen_t *xcbAppScreen = xcb_aux_get_screen(QX11Info::connection(), QX11Info::appScreen());

    if (xcb_cursor_context_new(QX11Info::connection(), xcbAppScreen, &xcbCursorCtx) >= 0) {
        QVector<QByteArray> cursorNames = {QByteArrayLiteral("cross"),
                                            QByteArrayLiteral("crosshair"),
                                            QByteArrayLiteral("diamond-cross"),
                                            QByteArrayLiteral("cross-reverse")};

        for (const auto &cursorName : cursorNames) {
            xcb_cursor_t cursor = xcb_cursor_load_cursor(xcbCursorCtx, cursorName.constData());
            if (cursor != XCB_CURSOR_NONE) {
                xcbCursor = cursor;
                break;
            }
        }
    }

    // grab the cursor
    xcb_grab_pointer_cookie_t grabPointerCookie = xcb_grab_pointer_unchecked(QX11Info::connection(), // xcb connection
                                                                             0, // deliver events to owner? nope
                                                                             QX11Info::appRootWindow(), // window to grab pointer for (root)
                                                                             XCB_EVENT_MASK_BUTTON_RELEASE, // which events do I want
                                                                             XCB_GRAB_MODE_SYNC, // pointer grab mode
                                                                             XCB_GRAB_MODE_ASYNC, // keyboard grab mode (why is this even here)
                                                                             XCB_NONE, // confine pointer to which window (none)
                                                                             xcbCursor, // cursor to change to for the duration of grab
                                                                             XCB_TIME_CURRENT_TIME // do this right now
    );
    XcbReplyPtr<xcb_grab_pointer_reply_t> grabPointerReply(xcb_grab_pointer_reply(QX11Info::connection(), grabPointerCookie, nullptr));

    // if the grab failed, take the screenshot right away
    if (grabPointerReply->status != XCB_GRAB_STATUS_SUCCESS) {
        doGrabNow(grabMode, includePointer, includeDecorations);
        return;
    }

    // fix things if our pointer grab causes a lockup and install our event filter
    m_nativeEventFilter->setCaptureOptions(grabMode, includePointer, includeDecorations);
    xcb_allow_events(QX11Info::connection(), XCB_ALLOW_SYNC_POINTER, XCB_TIME_CURRENT_TIME);
    qApp->installNativeEventFilter(m_nativeEventFilter.get());

    // done. clean stuff up
    xcb_cursor_context_free(xcbCursorCtx);
    xcb_free_cursor(QX11Info::connection(), xcbCursor);
}

#include "moc_PlatformXcb.cpp"
