/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "ImagePlatform.h"
// #include "VideoPlatform.h"

class ImagePlatformNull final : public ImagePlatform
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.spectacle.ImagePlatform" FILE "metadata.json")
    Q_INTERFACES(ImagePlatform)

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

// A default video platform implementation. Can be used for platforms that aren't supported.
// class VideoPlatformNull final : public VideoPlatform
// {
//     Q_OBJECT
//
// public:
//     explicit VideoPlatformNull(const QString &unavailableMessage = {}, QObject *parent = nullptr);
//
//     RecordingModes supportedRecordingModes() const override;
//     Formats supportedFormats() const override;
//     void startRecording(const QUrl &fileUrl, RecordingMode recordingMode, const QVariantMap &options, bool includePointer) override;
//     void finishRecording() override;
//
// private:
//     QString m_unavailableMessage;
// };
