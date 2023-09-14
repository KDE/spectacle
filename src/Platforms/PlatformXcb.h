/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "Platform.h"

#include <xcb/xcb.h>
#include <xcb/xcb_image.h>

#include <QPixmap>

class PlatformXcb final : public Platform
{
    Q_OBJECT

public:
    explicit PlatformXcb(QObject *parent = nullptr);
    ~PlatformXcb() override;

    GrabModes supportedGrabModes() const override final;
    ShutterModes supportedShutterModes() const override final;

public Q_SLOTS:
    void doGrab(Platform::ShutterMode shutterMode, Platform::GrabMode grabMode, bool includePointer, bool includeDecorations) override final;

private Q_SLOTS:
    void updateSupportedGrabModes();
    void handleKWinScreenshotReply(quint64 drawable);
    void doGrabNow(Platform::GrabMode grabMode, bool includePointer, bool includeDecorations);
    void doGrabOnClick(Platform::GrabMode grabMode, bool includePointer, bool includeDecorations);

private:
    inline void updateWindowTitle(xcb_window_t window);
    bool isKWinAvailable();
    QPoint getCursorPosition();
    QRect getDrawableGeometry(xcb_drawable_t drawable);
    xcb_window_t getWindowUnderCursor();
    xcb_window_t getTransientWindowParent(xcb_window_t childWindow, QRect &windowRectOut, bool includeDecorations);
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
    void grabActiveWindow(bool includePointer, bool includeDecorations);
    void grabWindowUnderCursor(bool includePointer, bool includeDecorations);
    void grabTransientWithParent(bool includePointer, bool includeDecorations);

    // on-click screenshot shutter support needs a native event filter in xcb
    class OnClickEventFilter;
    std::unique_ptr<OnClickEventFilter> m_nativeEventFilter;

    GrabModes m_grabModes;
};
