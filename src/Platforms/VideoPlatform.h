/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QFlags>
#include <QObject>
#include <QRect>
#include <variant>

class QScreen;

class VideoPlatform : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVector<RecordingMode> supportedRecordingModes READ supportedRecordingModes CONSTANT)
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY recordingChanged)

public:
    explicit VideoPlatform(QObject *parent = nullptr);
    ~VideoPlatform() override = default;

    enum RecordingMode {
        Screen, //< records a specific output, provided its QScreen::name()
        Window, //< records a specific window, provided its uuid
        Region, //< records the provided region rectangle
    };
    Q_ENUM(RecordingMode)
    using RecordingOption = std::variant<QScreen *, QRect, QString>;

    bool isRecording() const;
    virtual QVector<RecordingMode> supportedRecordingModes() const = 0;
    virtual QString extension() const = 0;

protected:
    void setRecording(bool recording);

public Q_SLOTS:
    virtual void startRecording(const QString &path, RecordingMode recordingMode, const RecordingOption &option, bool includePointer) = 0;
    virtual void finishRecording() = 0;

Q_SIGNALS:
    void recordingChanged(bool isRecording);
    void recordingSaved(const QString &path);

private:
    bool m_recording = false;
};
