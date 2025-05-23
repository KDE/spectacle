/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QBasicTimer>
#include <QElapsedTimer>
#include <QFlags>
#include <QObject>
#include <QRect>
#include <QUrl>
#include <QVariant>
#include <qqmlregistration.h>

class QScreen;

class VideoPlatform : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by SpectacleCore")

    Q_PROPERTY(RecordingModes supportedRecordingModes READ supportedRecordingModes NOTIFY supportedRecordingModesChanged)
    Q_PROPERTY(Formats supportedFormats READ supportedFormats NOTIFY supportedFormatsChanged)
    Q_PROPERTY(qint64 recordedTime READ recordedTime NOTIFY recordedTimeChanged)
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY recordingStateChanged)
    Q_PROPERTY(RecordingState recordingState READ recordingState NOTIFY recordingStateChanged)

public:
    explicit VideoPlatform(QObject *parent = nullptr);
    ~VideoPlatform() override = default;

    enum RecordingMode : char {
        NoRecordingModes = 0b000,
        Screen =           0b001, //< records a specific output, provided its QScreen::name()
        Window =           0b010, //< records a specific window, provided its uuid
        Region =           0b100, //< records the provided region rectangle
    };
    Q_FLAG(RecordingMode)
    Q_DECLARE_FLAGS(RecordingModes, RecordingMode)

    enum class RecordingState : char {
        NotRecording, //< Not recording anything
        Recording, //< Actively recording
        Rendering, //< Finishing the recording, no longer actively receiving frames but still processing to do.
        Finished //< Recording finished completely.
    };
    Q_ENUM(RecordingState)

    /**
     * Video formats supported by this class's APIs.
     *
     * Spectacle is supposed to be easy to use for screen recording. Encoders and containers are
     * very complex topics and there are many possible combinations, so we should try to keep the
     * format options simple. We should only add new formats or keep old formats if they fulfill
     * a usecase.
     *
     * Our definition of "best available encoder" is whichever encoder provides the best balance of
     * compatibility, performance and quality for most devices used by PC and smartphone users.
     *
     * We don't need to worry about ABI compatibility, but we do need to migrate user configs with
     * kconf_update if enum values change.
     *
     * Consider looking at guides like the following when adding new formats:
     *
     * - <https://developer.mozilla.org/en-US/docs/Web/Media/Formats/Video_codecs#recommendations_for_everyday_videos>
     */
    enum Format {
        /// A value for when no encoders are available.
        NoFormat = 0,
        /**
         * WebM container with the best available VP9 encoder.
         *
         * Advantages: Works OOTB on most Linux distros and well supported by web browsers.
         *
         * Disadvantages: Hardware accelerated encoding support is uncommon.
         *
         * If we add audio support, this should use Opus.
         */
        WebM_VP9 = 1,
        /**
         * MP4 container with the best available H.264 encoder.
         *
         * Advantages: Hardware accelerated encoding support is very common and well supported by
         * web browsers.
         *
         * Disadvantages: Doesn't work OOTB on all Linux distros. While it's still good, it's not as
         * efficient as newer formats.
         *
         * If we add audio support, this should use AAC.
         */
        MP4_H264 = 1 << 1,
        /**
         * WebP container with the best available WebP encoder.
         *
         * Advantages: Works OOTB on most Linux distros and well supported by web browsers.
         *
         * Disadvantages: No accelerated encoding support and no audio.
         */
        WebP = 1 << 2,
        /**
         * GIF container with the best available GIF encoder.
         *
         * Advantages: Works OOTB on most Linux distros and well supported by web browsers.
         *
         * Disadvantages: Terrible compression, no accelerated encoding support, no audio.
         */
        Gif = 1 << 3,
        /// Used to define the default format for settings
        DefaultFormat = WebM_VP9,
    };
    Q_FLAG(Format)
    Q_DECLARE_FLAGS(Formats, Format)

    virtual RecordingModes supportedRecordingModes() const = 0;

    virtual Formats supportedFormats() const = 0;

    static QString extensionForFormat(Format format);

    static Format formatForExtension(const QString &extension);
    Q_INVOKABLE static Format formatForPath(const QString &path);

    bool isRecording() const;
    qint64 recordedTime() const;

    RecordingState recordingState() const;

protected:
    void setRecordingState(RecordingState state);
    void timerEvent(QTimerEvent *event) override;

public Q_SLOTS:
    virtual void startRecording(const QUrl &fileUrl,
                                VideoPlatform::RecordingMode recordingMode,
                                const QVariantMap &options,
                                bool includePointer) = 0;
    virtual void finishRecording() = 0;

Q_SIGNALS:
    void supportedRecordingModesChanged();
    void supportedFormatsChanged();
    void recordingSaved(const QUrl &fileUrl);
    void recordingFailed(const QString &message);
    void recordingCanceled(const QString &message);
    void recordedTimeChanged();
    void recordingStateChanged(RecordingState state);

    /// Request a region from the platform agnostic selection editor
    void regionRequested();

private:
    QElapsedTimer m_elapsedTimer;
    QBasicTimer m_basicTimer;
    qint64 m_recordedTime = 0;
    RecordingState m_recordingState = RecordingState::NotRecording;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(VideoPlatform::RecordingModes)
Q_DECLARE_OPERATORS_FOR_FLAGS(VideoPlatform::Formats)
