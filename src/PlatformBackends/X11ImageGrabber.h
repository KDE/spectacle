/*
 *  Copyright (C) 2015 Boudhayan Gupta <me@BaloneyGeek.com>
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

#include <QStack>
#include <QX11Info>
#include <QString>
#include <QPixmap>
#include <QImage>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QCursor>
#include <QPoint>
#include <QRect>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QTimer>
#include <QVector>
#include <QByteArray>
#include <QProcessEnvironment>
#include <QScopedPointer>
#include <QScopedPointerPodDeleter>
#include <QAbstractNativeEventFilter>
#include <QMetaObject>

#include <KWindowSystem>
#include <KWindowInfo>
#include <KScreen/Config>
#include <KScreen/GetConfigOperation>
#include <KScreen/Screen>
#include <KScreen/Output>

#include <xcb/xcb.h>
#include <xcb/xcb_cursor.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_util.h>
#include <xcb/xfixes.h>

#include "ImageGrabber.h"

class X11ImageGrabber;

class OnClickEventFilter : public QAbstractNativeEventFilter
{
    public:

    explicit OnClickEventFilter(X11ImageGrabber *grabber);
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result);

    private:

    X11ImageGrabber *mImageGrabber;
};

class X11ImageGrabber : public ImageGrabber
{
    Q_OBJECT

    public:

    explicit X11ImageGrabber(QObject * parent = 0);
    ~X11ImageGrabber();

    bool onClickGrabSupported() const Q_DECL_OVERRIDE;

    protected:

    void grabFullScreen()          Q_DECL_OVERRIDE;
    void grabCurrentScreen()       Q_DECL_OVERRIDE;
    void grabActiveWindow()        Q_DECL_OVERRIDE;
    void grabRectangularRegion()   Q_DECL_OVERRIDE;
    void grabWindowUnderCursor()   Q_DECL_OVERRIDE;
    void grabTransientWithParent() Q_DECL_OVERRIDE;
    QPixmap blendCursorImage(const QPixmap &pixmap, int x, int y, int width, int height) Q_DECL_OVERRIDE;

    private slots:

    void KWinDBusScreenshotHelper(quint64 window);
    void KScreenCurrentMonitorScreenshotHelper(KScreen::ConfigOperation *op);
    void rectangleSelectionConfirmed(const QPixmap &pixmap, const QRect &region);
    void rectangleSelectionCancelled();

    public slots:

    void doOnClickGrab();

    private:

    bool                 isKWinAvailable();
    xcb_window_t         getRealWindowUnderCursor();
    void                 grabApplicationWindowHelper(xcb_window_t window);
    QRect                getApplicationWindowGeometry(xcb_window_t window);
    QStack<xcb_window_t> findAllChildren(xcb_window_t window);
    xcb_window_t         findParent(xcb_window_t window);
    QPixmap              getWindowPixmap(xcb_window_t window, bool blendPointer);
    QPixmap              convertFromNative(xcb_image_t *xcbImage);

    OnClickEventFilter          *mNativeEventFilter;
    KScreen::GetConfigOperation *mScreenConfigOperation;
};

template <typename T> using CScopedPointer = QScopedPointer<T, QScopedPointerPodDeleter>;

#endif // X11IMAGEGRABBER_H
