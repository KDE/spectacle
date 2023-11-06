/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "ImagePlatform.h"
#include "VideoPlatform.h"

class ImagePlatformNull final : public ImagePlatform
{
    Q_OBJECT

public:
    explicit ImagePlatformNull(QObject *parent = nullptr);
    ~ImagePlatformNull() override = default;

    GrabModes supportedGrabModes() const override final;
    ShutterModes supportedShutterModes() const override final;

public Q_SLOTS:

    void doGrab(ImagePlatform::ShutterMode shutterMode,
                ImagePlatform::GrabMode grabMode,
                bool includePointer,
                bool includeDecorations,
                bool includeShadow) override final;
};

class VideoPlatformNull final : public VideoPlatform
{
    Q_OBJECT

public:
    explicit VideoPlatformNull(QObject *parent = nullptr);

    RecordingModes supportedRecordingModes() const override;
    Formats supportedFormats() const override;
    void startRecording(const QUrl &fileUrl, RecordingMode recordingMode, const QVariant &option, bool includePointer) override;
    void finishRecording() override;

private:
    QUrl m_fileUrl;
};
