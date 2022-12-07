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

    QString platformName() const override final;
    GrabModes supportedGrabModes() const override final;
    ShutterModes supportedShutterModes() const override final;

public Q_SLOTS:
    void doGrab(Platform::ShutterMode theShutterMode, Platform::GrabMode theGrabMode, bool theIncludePointer, bool theIncludeDecorations) override final;

private Q_SLOTS:
    void updateSupportedGrabModes();
    void handleKWinScreenshotReply(quint64 theDrawable);
    void doGrabNow(Platform::GrabMode theGrabMode, bool theIncludePointer, bool theIncludeDecorations);
    void doGrabOnClick(Platform::GrabMode theGrabMode, bool theIncludePointer, bool theIncludeDecorations);

private:
    inline void updateWindowTitle(xcb_window_t theWindow);
    bool isKWinAvailable();
    QPoint getCursorPosition();
    QRect getDrawableGeometry(xcb_drawable_t theDrawable);
    xcb_window_t getWindowUnderCursor();
    xcb_window_t getTransientWindowParent(xcb_window_t theChildWindow, QRect &theWindowRectOut, bool theIncludeDecorations);
    QPixmap convertFromNative(xcb_image_t *theXcbImage);
    QPixmap blendCursorImage(QPixmap &thePixmap, const QRect theRect);
    QPixmap postProcessPixmap(QPixmap &thePixmap, QRect theRect, bool theBlendPointer);
    QPixmap getPixmapFromDrawable(xcb_drawable_t theXcbDrawable, const QRect &theRect);
    QPixmap getToplevelPixmap(QRect theRect, bool theBlendPointer);
    QPixmap getWindowPixmap(xcb_window_t theWindow, bool theBlendPointer);

    void grabAllScreens(bool theIncludePointer);
    void grabCurrentScreen(bool theIncludePointer);
    void grabApplicationWindow(xcb_window_t theWindow, bool theIncludePointer, bool theIncludeDecorations);
    void grabActiveWindow(bool theIncludePointer, bool theIncludeDecorations);
    void grabWindowUnderCursor(bool theIncludePointer, bool theIncludeDecorations);
    void grabTransientWithParent(bool theIncludePointer, bool theIncludeDecorations);

    // on-click screenshot shutter support needs a native event filter in xcb
    class OnClickEventFilter;
    std::unique_ptr<OnClickEventFilter> m_nativeEventFilter;

    GrabModes m_grabModes;
};
