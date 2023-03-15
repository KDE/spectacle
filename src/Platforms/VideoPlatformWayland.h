/*
    SPDX-FileCopyrightText: 2023 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "VideoPlatform.h"
#include <KPipeWire/PipeWireRecord>
#include <memory>

class Screencasting;

/**
 * The VideoPlatformWayland class uses the org.kde.KWin.ScreenShot2 dbus interface
 * for taking screenshots of screens and windows.
 */
class VideoPlatformWayland final : public VideoPlatform
{
    Q_OBJECT

public:
    VideoPlatformWayland(QObject *parent = nullptr);

    RecordingModes supportedRecordingModes() const override;
    void startRecording(const QString &path, RecordingMode recordingMode, const RecordingOption &option, bool includePointer) override;
    void finishRecording() override;
    QString extension() const override;
    QStringList suggestedExtensions() const override;
    void setExtension(const QString &encoder) override;

private:
    Screencasting *const m_screencasting;
    std::unique_ptr<PipeWireRecord> m_recorder;
};
