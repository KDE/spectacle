/* This file is part of Spectacle, the KDE screenshot utility
 * Copyright (C) 2019 Boudhayan Gupta <bgupta@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <QFlags>

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
        TransientWithParent = 0x10
    };
    using GrabModes = QFlags<GrabMode>;
    Q_FLAG(GrabModes)

    enum class ShutterMode {
        Immediate = 0x00,
        OnClick   = 0x01
    };
    using ShutterModes = QFlags<ShutterMode>;
    Q_FLAG(ShutterModes)

    explicit Platform(QObject *parent = nullptr);
    virtual ~Platform() = default;

    virtual QString platformName() const = 0;
    virtual GrabModes supportedGrabModes() const = 0;
    virtual ShutterModes supportedShutterModes() const = 0;

    public Q_SLOTS:

    virtual void doGrab(ShutterMode theShutterMode, GrabMode theGrabMode, bool theIncludePointer, bool theIncludeDecorations) = 0;

    Q_SIGNALS:

    void newScreenshotTaken(const QPixmap &thePixmap);
    void newScreenshotFailed();
    void windowTitleChanged(const QString &theWindowTitle);
};

Q_DECLARE_METATYPE(Platform::GrabMode)
Q_DECLARE_METATYPE(Platform::ShutterMode)
Q_DECLARE_OPERATORS_FOR_FLAGS(Platform::GrabModes)
Q_DECLARE_OPERATORS_FOR_FLAGS(Platform::ShutterModes)
