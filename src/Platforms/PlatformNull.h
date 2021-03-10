/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "Platform.h"

class PlatformNull final: public Platform
{
    Q_OBJECT

    public:

    explicit PlatformNull(QObject *parent = nullptr);
    virtual ~PlatformNull() = default;

    QString platformName() const override final;
    GrabModes supportedGrabModes() const override final;
    ShutterModes supportedShutterModes() const override final;

    public Q_SLOTS:

    void doGrab(Platform::ShutterMode theShutterMode, Platform::GrabMode theGrabMode, bool theIncludePointer, bool theIncludeDecorations) override final;
};
