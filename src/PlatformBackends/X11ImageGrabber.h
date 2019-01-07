/*
 *  Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
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

#ifndef X11IMAGEGRABBER_H
#define X11IMAGEGRABBER_H

#include <QAbstractNativeEventFilter>

#include <xcb/xcb.h>
#include <xcb/xcb_image.h>

#include "ImageGrabber.h"

class X11ImageGrabber;

class OnClickEventFilter : public QAbstractNativeEventFilter
{
    public:

    explicit OnClickEventFilter(X11ImageGrabber *grabber);
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) Q_DECL_OVERRIDE;

    private:

    X11ImageGrabber *mImageGrabber;
};

class X11ImageGrabber : public ImageGrabber
{
    Q_OBJECT

    public:

    explicit X11ImageGrabber(QObject * parent = nullptr);
    ~X11ImageGrabber() Q_DECL_OVERRIDE;

    QVector<ImageGrabber::GrabMode> supportedModes() const override { return {FullScreen, CurrentScreen, ActiveWindow, WindowUnderCursor, TransientWithParent, RectangularRegion}; }
    bool onClickGrabSupported() const Q_DECL_OVERRIDE;

    protected:

    void grabFullScreen()          Q_DECL_OVERRIDE;
    void grabCurrentScreen()       Q_DECL_OVERRIDE;
    void grabActiveWindow()        Q_DECL_OVERRIDE;
    void grabRectangularRegion()   Q_DECL_OVERRIDE;
    void grabWindowUnderCursor()   Q_DECL_OVERRIDE;
    void grabTransientWithParent() Q_DECL_OVERRIDE;
    QPixmap blendCursorImage(const QPixmap &pixmap, int x, int y, int width, int height) Q_DECL_OVERRIDE;

    private Q_SLOTS:

    void KWinDBusScreenshotHelper(quint64 window);
    void rectangleSelectionConfirmed(const QPixmap &pixmap);
    void rectangleSelectionCancelled();

    public Q_SLOTS:

    void doOnClickGrab() Q_DECL_OVERRIDE;

    private:

    bool                 isKWinAvailable();
    xcb_window_t         getRealWindowUnderCursor();
    void                 grabApplicationWindowHelper(xcb_window_t window);
    QRect                getDrawableGeometry(xcb_drawable_t drawable);
    QPixmap              postProcessPixmap(const QPixmap &pixmap, QRect rect, bool blendPointer);
    QPixmap              getPixmapFromDrawable(xcb_drawable_t drawableId, const QRect &rect);
    QPixmap              getToplevelPixmap(QRect rect, bool blendPointer);
    QPixmap              getWindowPixmap(xcb_window_t window, bool blendPointer);
    QPixmap              convertFromNative(xcb_image_t *xcbImage);
    xcb_window_t         getTransientWindowParent(xcb_window_t winId, QRect &outRect);
    QPoint               getNativeCursorPosition();

    OnClickEventFilter          *mNativeEventFilter;
    void updateWindowTitle(xcb_window_t window);
};

template <typename T> using CScopedPointer = QScopedPointer<T, QScopedPointerPodDeleter>;

struct ScopedPointerXcbImageDeleter
{
    static inline void cleanup(xcb_image_t *xcbImage) {
        if (xcbImage) {
            xcb_image_destroy(xcbImage);
        }
    }
};

#endif // X11IMAGEGRABBER_H
