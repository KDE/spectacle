/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2016 Martin Graesslin <mgraesslin@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "Platform.h"

class QDBusPendingCall;

class PlatformKWinWayland final : public Platform
{
    Q_OBJECT

public:
    explicit PlatformKWinWayland(QObject *parent = nullptr);
    ~PlatformKWinWayland() override = default;

    QString platformName() const override final;
    GrabModes supportedGrabModes() const override final;
    ShutterModes supportedShutterModes() const override final;

public Q_SLOTS:

    void doGrab(Platform::ShutterMode theShutterMode, Platform::GrabMode theGrabMode, bool theIncludePointer, bool theIncludeDecorations) override final;

private Q_SLOTS:
    void updateSupportedGrabModes();

private:
    void startReadImage(int theReadPipe);
    void startReadImages(int theReadPipe);
    void checkDbusPendingCall(const QDBusPendingCall &pcall);

    bool screenshotScreensAvailable() const;

    template<typename... ArgType>
    void callDBus(const QString &theGrabMethod, int theWriteFile, ArgType... arguments);

    template<typename... ArgType>
    void doGrabHelper(const QString &theGrabMethod, ArgType... arguments);
    template<typename... ArgType>
    void doGrabImagesHelper(const QString &theGrabMethod, ArgType... arguments);

    GrabModes m_grabModes;
};
