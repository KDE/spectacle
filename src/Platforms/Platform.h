/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <QFlags>

#include "QuickEditor/ComparableQPoint.h"

class Platform: public QObject
{
    Q_OBJECT

    public:

    enum class GrabMode {
        InvalidChoice       = 0x00,
        AllScreens          = 0x01,
        CurrentScreen       = 0x02,
        ActiveWindow        = 0x04,
        WindowUnderCursor   = 0x08,
        TransientWithParent = 0x10,
        AllScreensScaled    = 0x20,
        PerScreenImageNative= 0x40,
    };
    using GrabModes = QFlags<GrabMode>;
    Q_FLAG(GrabModes)

    enum class ShutterMode {
        Immediate = 0x01,
        OnClick   = 0x02
    };
    using ShutterModes = QFlags<ShutterMode>;
    Q_FLAG(ShutterModes)

    explicit Platform(QObject *parent = nullptr);
    virtual ~Platform() = default;

    virtual QString platformName() const = 0;
    virtual GrabModes supportedGrabModes() const = 0;
    virtual ShutterModes supportedShutterModes() const = 0;

    public Q_SLOTS:

    virtual void doGrab(Platform::ShutterMode theShutterMode, Platform::GrabMode theGrabMode, bool theIncludePointer, bool theIncludeDecorations) = 0;

    Q_SIGNALS:

    void newScreenshotTaken(const QPixmap &thePixmap);
    void newScreensScreenshotTaken(const QVector<QImage> &images);

    void newScreenshotFailed();
    void windowTitleChanged(const QString &theWindowTitle);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Platform::GrabModes)
Q_DECLARE_OPERATORS_FOR_FLAGS(Platform::ShutterModes)
