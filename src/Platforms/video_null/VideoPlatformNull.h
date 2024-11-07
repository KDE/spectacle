/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "VideoPlatform.h"

// A default video platform implementation. Can be used for platforms that aren't supported.
class VideoPlatformNull final : public VideoPlatform
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.spectacle.VideoPlatform" FILE "metadata.json")
    Q_INTERFACES(VideoPlatform)

public:
    explicit VideoPlatformNull(const QString &unavailableMessage = {}, QObject *parent = nullptr);

    RecordingModes supportedRecordingModes() const override;
    Formats supportedFormats() const override;
    void startRecording(const QUrl &fileUrl, RecordingMode recordingMode, const QVariantMap &options, bool includePointer) override;
    void finishRecording() override;

private:
    QString m_unavailableMessage;
};
