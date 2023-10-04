/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "Platform.h"
#include "VideoPlatform.h"

class PlatformNull final : public Platform
{
    Q_OBJECT

public:
    explicit PlatformNull(QObject *parent = nullptr);
    ~PlatformNull() override = default;

    GrabModes supportedGrabModes() const override final;
    ShutterModes supportedShutterModes() const override final;

public Q_SLOTS:

    void doGrab(Platform::ShutterMode shutterMode, Platform::GrabMode grabMode, bool includePointer, bool includeDecorations) override final;
};

class VideoPlatformNull final : public VideoPlatform
{
    Q_OBJECT

public:
    explicit VideoPlatformNull(QObject *parent = nullptr);

    RecordingModes supportedRecordingModes() const override;
    Formats supportedFormats() const override;
    void startRecording(const QString &path, RecordingMode recordingMode, const RecordingOption &option, bool includePointer) override;
    void finishRecording() override;

private:
    QString m_path;
};
