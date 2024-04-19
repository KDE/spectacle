/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "ImagePlatform.h"

#include <xcb/xcb.h>
#include <xcb/xcb_image.h>

#include <QPixmap>

class ImagePlatformXcb final : public ImagePlatform
{
    Q_OBJECT

public:
    explicit ImagePlatformXcb(QObject *parent = nullptr);
    ~ImagePlatformXcb() override;

    GrabModes supportedGrabModes() const override final;
    ShutterModes supportedShutterModes() const override final;

public Q_SLOTS:
    void doGrab(ImagePlatform::ShutterMode shutterMode,
                ImagePlatform::GrabMode grabMode,
                bool includePointer,
                bool includeDecorations,
                bool includeShadow) override final;

private Q_SLOTS:
    void updateSupportedGrabModes();
    void doGrabNow(ImagePlatform::GrabMode grabMode, bool includePointer, bool includeDecorations, bool includeShadow);
    void doGrabOnClick(ImagePlatform::GrabMode grabMode, bool includePointer, bool includeDecorations, bool includeShadow);

private:
    inline void updateWindowTitle(xcb_window_t window);

    QPoint getCursorPosition();
    QRect getDrawableGeometry(xcb_drawable_t drawable);
    xcb_window_t getWindowUnderCursor();
    xcb_window_t getTransientWindowParent(xcb_window_t childWindow, QRect &windowRectOut, bool includeDecorations);

    /* ----------------------- Image Processing Utilities ----------------------- */

    /**
     * @brief Adds a drop shadow to the given image.
     * @param image The image to add a drop shadow to.
     * @return The image with a drop shadow.
     */
    QImage addDropShadow(QImage &image);

    QList<QRect> getScreenRects();
    QImage convertFromNative(xcb_image_t *xcbImage);
    QImage blendCursorImage(QImage &image, const QRect rect);
    QImage postProcessImage(QImage &image, QRect rect, bool blendPointer);
    QImage getImageFromDrawable(xcb_drawable_t xcbDrawable, const QRect &rect);
    QImage getToplevelImage(QRect rect, bool blendPointer);
    QImage getWindowImage(xcb_window_t window, bool blendPointer);

    void grabAllScreens(bool includePointer, bool crop = false);
    void grabCurrentScreen(bool includePointer);
    void grabApplicationWindow(xcb_window_t window, bool includePointer, bool includeDecorations);
    void grabActiveWindow(bool includePointer, bool includeDecorations, bool includeShadow);
    void grabWindowUnderCursor(bool includePointer, bool includeDecorations, bool includeShadow);
    void grabTransientWithParent(bool includePointer, bool includeDecorations, bool includeShadow);

    // on-click screenshot shutter support needs a native event filter in xcb
    class OnClickEventFilter;
    std::unique_ptr<OnClickEventFilter> m_nativeEventFilter;

    GrabModes m_grabModes;
};
