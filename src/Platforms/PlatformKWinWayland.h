/* This file is part of Spectacle, the KDE screenshot utility
 * Copyright (C) 2016 Martin Graesslin <mgraesslin@kde.org>
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

#include "Platform.h"

class PlatformKWinWayland final: public Platform
{
    Q_OBJECT

    public:

    explicit PlatformKWinWayland(QObject *parent = nullptr);
    virtual ~PlatformKWinWayland() = default;

    QString platformName() const override final;
    GrabModes supportedGrabModes() const override final;
    ShutterModes supportedShutterModes() const override final;

    public Q_SLOTS:

    void doGrab(ShutterMode theShutterMode, GrabMode theGrabMode, bool theIncludePointer, bool theIncludeDecorations) override final;

    private:

    void startReadImage(int theReadPipe);
    template <typename ArgType> void doGrabHelper(const QString &theGrabMethod, ArgType theArgument);
    template <typename ArgType> void callDBus(const QString &theGrabMethod, ArgType theArgument, int theWriteFile);
};
