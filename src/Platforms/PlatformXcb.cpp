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
    void operator()(xcb_image_t *theXcbImage) const
    {
        if (theXcbImage) {
            xcb_image_destroy(theXcbImage);
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
    explicit OnClickEventFilter(PlatformXcb *thePlatformPtr)
        : mPlatformPtr(thePlatformPtr)
    {
    }

    void setCaptureOptions(Platform::GrabMode theGrabMode, bool theIncludePointer, bool theIncludeDecorations)
    {
        mGrabMode = theGrabMode;
        mIncludePointer = theIncludePointer;
        mIncludeDecorations = theIncludeDecorations;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    bool nativeEventFilter(const QByteArray &theEventType, void *theMessage, long * /* theResult */) override
#else
    bool nativeEventFilter(const QByteArray &theEventType, void *theMessage, qintptr * /*theResult*/) override
#endif
    {
        if (theEventType == "xcb_generic_event_t") {
            auto lFirstEvent = static_cast<xcb_generic_event_t *>(theMessage);

            switch (lFirstEvent->response_type & ~0x80) {
            case XCB_BUTTON_RELEASE: {
                // uninstall the eventfilter and release the mouse
                qApp->removeNativeEventFilter(this);
                xcb_ungrab_pointer(QX11Info::connection(), XCB_TIME_CURRENT_TIME);

                // decide whether to grab or abort. regrab the mouse on mouse-wheel events
                {
                    auto lSecondEvent = static_cast<xcb_button_release_event_t *>(theMessage);
                    if (lSecondEvent->detail == 1) {
                        QTimer::singleShot(0, nullptr, [this]() {
                            mPlatformPtr->doGrabNow(mGrabMode, mIncludePointer, mIncludeDecorations);
                        });
                    } else if (lSecondEvent->detail == 2 || lSecondEvent->detail == 3) {
                        // 2: middle click, 3: right click; both cancel
                        Q_EMIT mPlatformPtr->newScreenshotTaken(QPixmap());
                    } else if (lSecondEvent->detail < 4) {
                        Q_EMIT mPlatformPtr->newScreenshotFailed();
                    } else {
                        QTimer::singleShot(0, nullptr, [this]() {
                            mPlatformPtr->doGrabOnClick(mGrabMode, mIncludePointer, mIncludeDecorations);
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
    PlatformXcb *mPlatformPtr;
    Platform::GrabMode mGrabMode{GrabMode::AllScreens};
    bool mIncludePointer{true};
    bool mIncludeDecorations{true};
};

/* -- General Plumbing ------------------------------------------------------------------------- */

PlatformXcb::PlatformXcb(QObject *theParent)
    : Platform(theParent)
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

void PlatformXcb::doGrab(ShutterMode theShutterMode, GrabMode theGrabMode, bool theIncludePointer, bool theIncludeDecorations)
{
    switch (theShutterMode) {
    case ShutterMode::Immediate: {
        doGrabNow(theGrabMode, theIncludePointer, theIncludeDecorations);
        return;
    }
    case ShutterMode::OnClick: {
        doGrabOnClick(theGrabMode, theIncludePointer, theIncludeDecorations);
        return;
    }
    }
}

/* -- Platform Utilities ----------------------------------------------------------------------- */

void PlatformXcb::updateWindowTitle(xcb_window_t theWindow)
{
    auto lTitle = KX11Extras::readNameProperty(theWindow, XA_WM_NAME);
    Q_EMIT windowTitleChanged(lTitle);
}

bool PlatformXcb::isKWinAvailable()
{
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.KWin"))) {
        QDBusInterface lIface(QStringLiteral("org.kde.KWin"), QStringLiteral("/Effects"), QStringLiteral("org.kde.kwin.Effects"));
        QDBusReply<bool> lReply = lIface.call(QStringLiteral("isEffectLoaded"), QStringLiteral("screenshot"));
        return lReply.value();
    }
    return false;
}

/* -- XCB Utilities ---------------------------------------------------------------------------- */

QPoint PlatformXcb::getCursorPosition()
{
    // QCursor::pos() is not used because it requires additional calculations.
    // Its value is the offset to the origin of the current screen is in
    // device-independent pixels while the origin itself uses native pixels.

    auto lXcbConn = QX11Info::connection();
    auto lPointerCookie = xcb_query_pointer_unchecked(lXcbConn, QX11Info::appRootWindow());
    XcbReplyPtr<xcb_query_pointer_reply_t> lPointerReply(xcb_query_pointer_reply(lXcbConn, lPointerCookie, nullptr));

    return QPoint(lPointerReply->root_x, lPointerReply->root_y);
}

QRect PlatformXcb::getDrawableGeometry(xcb_drawable_t theDrawable)
{
    auto lXcbConn = QX11Info::connection();
    auto lGeoCookie = xcb_get_geometry_unchecked(lXcbConn, theDrawable);
    XcbReplyPtr<xcb_get_geometry_reply_t> lGeoReply(xcb_get_geometry_reply(lXcbConn, lGeoCookie, nullptr));
    if (!lGeoReply) {
        return QRect();
    }
    return QRect(lGeoReply->x, lGeoReply->y, lGeoReply->width, lGeoReply->height);
}

xcb_window_t PlatformXcb::getWindowUnderCursor()
{
    auto lXcbConn = QX11Info::connection();
    auto lAppWin = QX11Info::appRootWindow();

    const QByteArray lAtomName("WM_STATE");
    auto lAtomCookie = xcb_intern_atom_unchecked(lXcbConn, 0, lAtomName.length(), lAtomName.constData());
    auto lPointerCookie = xcb_query_pointer_unchecked(lXcbConn, lAppWin);
    XcbReplyPtr<xcb_intern_atom_reply_t> lAtomReply(xcb_intern_atom_reply(lXcbConn, lAtomCookie, nullptr));
    XcbReplyPtr<xcb_query_pointer_reply_t> lPointerReply(xcb_query_pointer_reply(lXcbConn, lPointerCookie, nullptr));

    if (lAtomReply->atom == XCB_ATOM_NONE) {
        return QX11Info::appRootWindow();
    }

    // now start testing
    QStack<xcb_window_t> lWindowStack;
    lWindowStack.push(lPointerReply->child);

    while (!lWindowStack.isEmpty()) {
        lAppWin = lWindowStack.pop();

        // next, check if our window has the WM_STATE property set on
        // the window. if yes, return the window - we have found it
        auto lPropCookie = xcb_get_property_unchecked(lXcbConn, 0, lAppWin, lAtomReply->atom, XCB_ATOM_ANY, 0, 0);
        XcbReplyPtr<xcb_get_property_reply_t> lPropReply(xcb_get_property_reply(lXcbConn, lPropCookie, nullptr));

        if (lPropReply->type != XCB_ATOM_NONE) {
            return lAppWin;
        }

        // if we're here, this means the window is not the real window
        // we should start looking at its children
        auto lTreeCookie = xcb_query_tree_unchecked(lXcbConn, lAppWin);
        XcbReplyPtr<xcb_query_tree_reply_t> lTreeReply(xcb_query_tree_reply(lXcbConn, lTreeCookie, nullptr));
        auto lWindowChildren = xcb_query_tree_children(lTreeReply.get());
        auto lWindowChildrenLength = xcb_query_tree_children_length(lTreeReply.get());

        for (int iIdx = lWindowChildrenLength - 1; iIdx >= 0; iIdx--) {
            lWindowStack.push(lWindowChildren[iIdx]);
        }
    }

    // return the window. it has geometry information for a crop
    return lPointerReply->child;
}

xcb_window_t PlatformXcb::getTransientWindowParent(xcb_window_t theChildWindow, QRect &theWindowRectOut, bool theIncludeDecorations)
{
    NET::Properties lNetProp = theIncludeDecorations ? NET::WMFrameExtents : NET::WMGeometry;
    KWindowInfo lWindowInfo(theChildWindow, lNetProp, NET::WM2TransientFor);

    // add the current window to the image
    if (theIncludeDecorations) {
        theWindowRectOut = lWindowInfo.frameGeometry();
    } else {
        theWindowRectOut = lWindowInfo.geometry();
    }
    return lWindowInfo.transientFor();
}

/* -- Image Processing Utilities --------------------------------------------------------------- */

QPixmap PlatformXcb::convertFromNative(xcb_image_t *theXcbImage)
{
    auto lImageFormat = QImage::Format_Invalid;
    switch (theXcbImage->depth) {
    case 1:
        lImageFormat = QImage::Format_MonoLSB;
        break;
    case 16:
        lImageFormat = QImage::Format_RGB16;
        break;
    case 24:
        lImageFormat = QImage::Format_RGB32;
        break;
    case 30:
        lImageFormat = QImage::Format_RGB30;
        break;
    case 32:
        lImageFormat = QImage::Format_ARGB32_Premultiplied;
        break;
    default:
        return QPixmap(); // we don't know
    }

    // the RGB32 format requires data format 0xffRRGGBB, ensure that this fourth byte really is 0xff
    if (lImageFormat == QImage::Format_RGB32) {
        auto lData = reinterpret_cast<quint32 *>(theXcbImage->data);
        for (size_t iIter = 0; iIter < theXcbImage->width * theXcbImage->height; iIter++) {
            lData[iIter] |= 0xff000000;
        }
    }

    QImage lImage(theXcbImage->data, theXcbImage->width, theXcbImage->height, lImageFormat);
    if (lImage.isNull()) {
        return QPixmap();
    }

    // work around an abort in QImage::color
    if (lImage.format() == QImage::Format_MonoLSB) {
        lImage.setColorCount(2);
        lImage.setColor(0, QColor(Qt::white).rgb());
        lImage.setColor(1, QColor(Qt::black).rgb());
    }

    // the image is ready. Since the backing data from xcbImage could be freed
    // before the QPixmap goes away, a deep copy is necessary.
    return QPixmap::fromImage(lImage).copy();
}

QPixmap PlatformXcb::blendCursorImage(QPixmap &thePixmap, const QRect theRect)
{
    // If the cursor position lies outside the area, do not bother drawing a cursor.

    auto lCursorPos = getCursorPosition();
    if (!theRect.contains(lCursorPos)) {
        return thePixmap;
    }

    // now we can get the image and start processing
    auto lXcbConn = QX11Info::connection();

    auto lCursorCookie = xcb_xfixes_get_cursor_image_unchecked(lXcbConn);
    XcbReplyPtr<xcb_xfixes_get_cursor_image_reply_t> lCursorReply(xcb_xfixes_get_cursor_image_reply(lXcbConn, lCursorCookie, nullptr));
    if (!lCursorReply) {
        return thePixmap;
    }

    // get the image and process it into a qimage
    auto lPixelData = xcb_xfixes_get_cursor_image_cursor_image(lCursorReply.get());
    if (!lPixelData) {
        return thePixmap;
    }
    QImage lCursorImage(reinterpret_cast<quint8 *>(lPixelData), lCursorReply->width, lCursorReply->height, QImage::Format_ARGB32_Premultiplied);

    // a small fix for the cursor position for fancier cursors
    lCursorPos -= QPoint(lCursorReply->xhot, lCursorReply->yhot);

    // now we translate the cursor point to our screen rectangle and do the painting
    lCursorPos -= QPoint(theRect.x(), theRect.y());
    QPainter lPainter(&thePixmap);
    lPainter.drawImage(lCursorPos, lCursorImage);
    return thePixmap;
}

QPixmap PlatformXcb::postProcessPixmap(QPixmap &thePixmap, QRect theRect, bool theBlendPointer)
{
    if (!(theBlendPointer)) {
        // note: this may be the null pixmap if an error occurred.
        return thePixmap;
    }
    return blendCursorImage(thePixmap, theRect);
}

/* -- Capture Helpers -------------------------------------------------------------------------- */

QPixmap PlatformXcb::getPixmapFromDrawable(xcb_drawable_t theXcbDrawable, const QRect &theRect)
{
    auto lXcbConn = QX11Info::connection();

    // proceed to get an image based on the geometry (in device pixels)
    XcbImagePtr lXcbImage(xcb_image_get(lXcbConn, theXcbDrawable, theRect.x(), theRect.y(), theRect.width(), theRect.height(), ~0, XCB_IMAGE_FORMAT_Z_PIXMAP));

    // too bad, the capture failed.
    if (!lXcbImage) {
        return QPixmap();
    }

    // now process the image
    auto lPixmap = convertFromNative(lXcbImage.get());
    return lPixmap;
}

QPixmap PlatformXcb::getToplevelPixmap(QRect theRect, bool theBlendPointer)
{
    auto lRootWindow = QX11Info::appRootWindow();

    // treat a null rect as an alias for capturing fullscreen
    if (!theRect.isValid()) {
        theRect = getDrawableGeometry(lRootWindow);
    } else {
        QRegion lScreenRegion;
        const auto lScreens = QGuiApplication::screens();
        for (auto lScreen : lScreens) {
            auto lScreenRect = lScreen->geometry();

            // Do not use setSize() here, because QSize::operator*=()
            // performs qRound() which can result in xcb_image_get() failing
            const auto lPixelRatio = lScreen->devicePixelRatio();
            lScreenRect.setHeight(qFloor(lScreenRect.height() * lPixelRatio));
            lScreenRect.setWidth(qFloor(lScreenRect.width() * lPixelRatio));

            lScreenRegion += lScreenRect;
        }
        theRect = (lScreenRegion & theRect).boundingRect();
    }

    auto lPixmap = getPixmapFromDrawable(lRootWindow, theRect);
    return postProcessPixmap(lPixmap, theRect, theBlendPointer);
}

QPixmap PlatformXcb::getWindowPixmap(xcb_window_t theWindow, bool theBlendPointer)
{
    auto lXcbConn = QX11Info::connection();

    // first get geometry information for our window
    auto lGeoCookie = xcb_get_geometry_unchecked(lXcbConn, theWindow);
    XcbReplyPtr<xcb_get_geometry_reply_t> lGeoReply(xcb_get_geometry_reply(lXcbConn, lGeoCookie, nullptr));
    QRect lWindowRect(lGeoReply->x, lGeoReply->y, lGeoReply->width, lGeoReply->height);

    // then proceed to get an image
    auto lPixmap = getPixmapFromDrawable(theWindow, lWindowRect);

    // translate window coordinates to global ones.
    auto lRootGeoCookie = xcb_get_geometry_unchecked(lXcbConn, lGeoReply->root);
    XcbReplyPtr<xcb_get_geometry_reply_t> lRootGeoReply(xcb_get_geometry_reply(lXcbConn, lRootGeoCookie, nullptr));
    auto lTranslateCookie = xcb_translate_coordinates_unchecked(lXcbConn, theWindow, lGeoReply->root, lRootGeoReply->x, lRootGeoReply->y);
    XcbReplyPtr<xcb_translate_coordinates_reply_t> lTranslateReply(xcb_translate_coordinates_reply(lXcbConn, lTranslateCookie, nullptr));

    // adjust local to global coordinates.
    lWindowRect.moveRight(lWindowRect.x() + lTranslateReply->dst_x);
    lWindowRect.moveTop(lWindowRect.y() + lTranslateReply->dst_y);

    // if the window capture failed, try to obtain one from the full screen.
    if (lPixmap.isNull()) {
        return getToplevelPixmap(lWindowRect, theBlendPointer);
    }
    return postProcessPixmap(lPixmap, lWindowRect, theBlendPointer);
}

void PlatformXcb::handleKWinScreenshotReply(quint64 theDrawable)
{
    QDBusConnection::sessionBus().disconnect(QStringLiteral("org.kde.KWin"),
                                             QStringLiteral("/Screenshot"),
                                             QStringLiteral("org.kde.kwin.Screenshot"),
                                             QStringLiteral("screenshotCreated"),
                                             this,
                                             SLOT(handleKWinScreenshotReply(quint64)));

    // obtain width and height and grab an image (x and y are always zero for pixmaps)
    auto lDrawable = static_cast<xcb_drawable_t>(theDrawable);
    auto lRect = getDrawableGeometry(lDrawable);
    auto lPixmap = getPixmapFromDrawable(lDrawable, lRect);

    if (!lPixmap.isNull()) {
        Q_EMIT newScreenshotTaken(lPixmap);
        return;
    }
    Q_EMIT newScreenshotFailed();
}

/* -- Grabber Methods -------------------------------------------------------------------------- */

void PlatformXcb::grabAllScreens(bool theIncludePointer)
{
    auto lPixmap = getToplevelPixmap(QRect(), theIncludePointer);
    Q_EMIT newScreenshotTaken(lPixmap);
}

void PlatformXcb::grabCurrentScreen(bool theIncludePointer)
{
    auto lCursorPosition = QCursor::pos();
    const auto lScreens = QGuiApplication::screens();
    for (auto lScreen : lScreens) {
        auto lScreenRect = lScreen->geometry();
        if (!lScreenRect.contains(lCursorPosition)) {
            continue;
        }

        // the screen origin is in native pixels, but the size is device-dependent.
        // convert these also to native pixels.
        QRect lNativeScreenRect(lScreenRect.topLeft(), lScreenRect.size() * lScreen->devicePixelRatio());
        auto lPixmap = getToplevelPixmap(lNativeScreenRect, theIncludePointer);
        Q_EMIT newScreenshotTaken(lPixmap);
        return;
    }

    // no screen found with our cursor, fallback to capturing all screens
    grabAllScreens(theIncludePointer);
}

void PlatformXcb::grabApplicationWindow(xcb_window_t theWindow, bool theIncludePointer, bool theIncludeDecorations)
{
    // if the user doesn't want decorations captured, we're in luck. This is
    // the easiest bit

    auto lPixmap = getWindowPixmap(theWindow, theIncludePointer);
    if (!theIncludeDecorations || theWindow == QX11Info::appRootWindow()) {
        Q_EMIT newScreenshotTaken(lPixmap);
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

    KWindowInfo lWindowInfo(theWindow, NET::WMFrameExtents);
    if (lWindowInfo.valid()) {
        auto lFrameGeom = lWindowInfo.frameGeometry();
        lPixmap = getToplevelPixmap(lFrameGeom, theIncludePointer);
    }

    // fallback is window without the frame
    Q_EMIT newScreenshotTaken(lPixmap);
}

void PlatformXcb::grabActiveWindow(bool theIncludePointer, bool theIncludeDecorations)
{
    auto lActiveWindow = KX11Extras::activeWindow();
    updateWindowTitle(lActiveWindow);

    // if KWin is available, use the KWin DBus interfaces
    if (theIncludeDecorations && isKWinAvailable()) {
        auto lBus = QDBusConnection::sessionBus();
        lBus.connect(QStringLiteral("org.kde.KWin"),
                     QStringLiteral("/Screenshot"),
                     QStringLiteral("org.kde.kwin.Screenshot"),
                     QStringLiteral("screenshotCreated"),
                     this,
                     SLOT(handleKWinScreenshotReply(quint64)));
        QDBusInterface lIface(QStringLiteral("org.kde.KWin"), QStringLiteral("/Screenshot"), QStringLiteral("org.kde.kwin.Screenshot"));

        int lOpMask = 1;
        if (theIncludePointer) {
            lOpMask |= 1 << 1;
        }
        lIface.call(QStringLiteral("screenshotForWindow"), static_cast<quint64>(lActiveWindow), lOpMask);

        return;
    }

    // otherwise, use the native functionality
    grabApplicationWindow(lActiveWindow, theIncludePointer, theIncludeDecorations);
}

void PlatformXcb::grabWindowUnderCursor(bool theIncludePointer, bool theIncludeDecorations)
{
    auto lWindow = getWindowUnderCursor();
    updateWindowTitle(lWindow);

    // if KWin is available, use the KWin DBus interfaces
    if (theIncludeDecorations && isKWinAvailable()) {
        auto lBus = QDBusConnection::sessionBus();
        lBus.connect(QStringLiteral("org.kde.KWin"),
                     QStringLiteral("/Screenshot"),
                     QStringLiteral("org.kde.kwin.Screenshot"),
                     QStringLiteral("screenshotCreated"),
                     this,
                     SLOT(handleKWinScreenshotReply(quint64)));
        QDBusInterface lInterface(QStringLiteral("org.kde.KWin"), QStringLiteral("/Screenshot"), QStringLiteral("org.kde.kwin.Screenshot"));

        int lOpMask = 1;
        if (theIncludePointer) {
            lOpMask |= 1 << 1;
        }
        lInterface.call(QStringLiteral("screenshotWindowUnderCursor"), lOpMask);

        return;
    }

    // otherwise, use the native functionality
    grabApplicationWindow(lWindow, theIncludePointer, theIncludeDecorations);
}

void PlatformXcb::grabTransientWithParent(bool theIncludePointer, bool theIncludeDecorations)
{
    auto lWindow = getWindowUnderCursor();
    updateWindowTitle(lWindow);

    // grab the image early
    auto lPixmap = getToplevelPixmap(QRect(), false);

    // now that we know we have a transient window, let's
    // find other possible transient windows and the app window itself.
    QRegion lClipRegion;
    QSet<xcb_window_t> lTransientWindows;
    auto lParentWindow = lWindow;
    const QRect lDesktopRect(0, 0, 1, 1);
    do {
        // find parent window and add the window to the visible region
        auto lWinId = lParentWindow;
        QRect lWinRect;
        lParentWindow = getTransientWindowParent(lWinId, lWinRect, theIncludeDecorations);
        lTransientWindows << lWinId;

        // Don't include the 1x1 pixel sized desktop window in the top left corner that is present
        // if the window is a QDialog without a parent.
        // BUG: 376350
        if (lWinRect != lDesktopRect) {
            lClipRegion += lWinRect;
        }

        // Continue walking only if this is a transient window (having a parent)
    } while (lParentWindow != XCB_WINDOW_NONE && !lTransientWindows.contains(lParentWindow));

    // All parents are known now, find other transient children.
    // Assume that the lowest window is behind everything else, then if a new
    // transient window is discovered, its children can then also be found.
    auto lWinList = KX11Extras::stackingOrder();
    for (auto lWinId : lWinList) {
        QRect lWinRect;
        auto lParentWindow = getTransientWindowParent(lWinId, lWinRect, theIncludeDecorations);

        // if the parent should be displayed, then show the child too
        if (lTransientWindows.contains(lParentWindow)) {
            if (!lTransientWindows.contains(lWinId)) {
                lTransientWindows << lWinId;
                lClipRegion += lWinRect;
            }
        }
    }

    // we can probably go ahead and generate the image now
    QImage lImage(lPixmap.size(), QImage::Format_ARGB32);
    lImage.fill(Qt::transparent);

    QPainter lPainter(&lImage);
    lPainter.setClipRegion(lClipRegion);
    lPainter.drawPixmap(0, 0, lPixmap);
    lPainter.end();
    lPixmap = QPixmap::fromImage(lImage).copy(lClipRegion.boundingRect());

    // why stop here, when we can render a 20px drop shadow all around it
    auto lShadowEffect = new QGraphicsDropShadowEffect;
    lShadowEffect->setOffset(0);
    lShadowEffect->setBlurRadius(20);

    auto lPixmapItem = new QGraphicsPixmapItem;
    lPixmapItem->setPixmap(lPixmap);
    lPixmapItem->setGraphicsEffect(lShadowEffect);

    QImage lShadowImage(lPixmap.size() + QSize(40, 40), QImage::Format_ARGB32);
    lShadowImage.fill(Qt::transparent);
    QPainter lShadowPainter(&lShadowImage);

    QGraphicsScene lGraphicsScene;
    lGraphicsScene.addItem(lPixmapItem);
    lGraphicsScene.render(&lShadowPainter, QRectF(), QRectF(-20, -20, lPixmap.width() + 40, lPixmap.height() + 40));
    lShadowPainter.end();

    // we can finish up now
    lPixmap = QPixmap::fromImage(lShadowImage);
    if (theIncludePointer) {
        auto lTopLeft = lClipRegion.boundingRect().topLeft() - QPoint(20, 20);
        lPixmap = blendCursorImage(lPixmap, QRect(lTopLeft, QSize(lPixmap.width(), lPixmap.height())));
    }
    Q_EMIT newScreenshotTaken(lPixmap);
}

void PlatformXcb::doGrabNow(GrabMode theGrabMode, bool theIncludePointer, bool theIncludeDecorations)
{
    if (theGrabMode & ~(ActiveWindow | WindowUnderCursor | TransientWithParent)) {
        // Notify that window title is empty since we are not picking a window.
        Q_EMIT windowTitleChanged();
    }
    switch (theGrabMode) {
    case GrabMode::AllScreens:
    case GrabMode::AllScreensScaled:
        grabAllScreens(theIncludePointer);
        break;
    case GrabMode::PerScreenImageNative: {
        auto lPixmap = getToplevelPixmap(QRect(), theIncludePointer);
        // break thePixmap into list of images
        const auto screens = QGuiApplication::screens();
        QVector<ScreenImage> screenImages;
        for (const auto screen : screens) {
            QRect geom = screen->geometry();
            geom.setSize(screen->size() * screen->devicePixelRatio());
            screenImages.append({screen, lPixmap.copy(geom).toImage(), screen->devicePixelRatio()});
        }
        Q_EMIT newScreensScreenshotTaken(screenImages);
        break;
    }
    case GrabMode::CurrentScreen:
        grabCurrentScreen(theIncludePointer);
        break;
    case GrabMode::ActiveWindow:
        grabActiveWindow(theIncludePointer, theIncludeDecorations);
        break;
    case GrabMode::WindowUnderCursor:
        grabWindowUnderCursor(theIncludePointer, theIncludeDecorations);
        break;
    case GrabMode::TransientWithParent:
        grabTransientWithParent(theIncludePointer, theIncludeDecorations);
        break;
    case GrabMode::InvalidChoice:
        Q_EMIT newScreenshotFailed();
    }
}

void PlatformXcb::doGrabOnClick(GrabMode theGrabMode, bool theIncludePointer, bool theIncludeDecorations)
{
    // get the cursor image
    xcb_cursor_t lXcbCursor = XCB_CURSOR_NONE;
    xcb_cursor_context_t *lXcbCursorCtx = nullptr;
    xcb_screen_t *lXcbAppScreen = xcb_aux_get_screen(QX11Info::connection(), QX11Info::appScreen());

    if (xcb_cursor_context_new(QX11Info::connection(), lXcbAppScreen, &lXcbCursorCtx) >= 0) {
        QVector<QByteArray> lCursorNames = {QByteArrayLiteral("cross"),
                                            QByteArrayLiteral("crosshair"),
                                            QByteArrayLiteral("diamond-cross"),
                                            QByteArrayLiteral("cross-reverse")};

        for (const auto &lCursorName : lCursorNames) {
            xcb_cursor_t lCursor = xcb_cursor_load_cursor(lXcbCursorCtx, lCursorName.constData());
            if (lCursor != XCB_CURSOR_NONE) {
                lXcbCursor = lCursor;
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
                                                                             lXcbCursor, // cursor to change to for the duration of grab
                                                                             XCB_TIME_CURRENT_TIME // do this right now
    );
    XcbReplyPtr<xcb_grab_pointer_reply_t> lGrabPointerReply(xcb_grab_pointer_reply(QX11Info::connection(), grabPointerCookie, nullptr));

    // if the grab failed, take the screenshot right away
    if (lGrabPointerReply->status != XCB_GRAB_STATUS_SUCCESS) {
        doGrabNow(theGrabMode, theIncludePointer, theIncludeDecorations);
        return;
    }

    // fix things if our pointer grab causes a lockup and install our event filter
    m_nativeEventFilter->setCaptureOptions(theGrabMode, theIncludePointer, theIncludeDecorations);
    xcb_allow_events(QX11Info::connection(), XCB_ALLOW_SYNC_POINTER, XCB_TIME_CURRENT_TIME);
    qApp->installNativeEventFilter(m_nativeEventFilter.get());

    // done. clean stuff up
    xcb_cursor_context_free(lXcbCursorCtx);
    xcb_free_cursor(QX11Info::connection(), lXcbCursor);
}
