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
#include <variant>

class QScreen;

class VideoPlatform : public QObject
{
    Q_OBJECT
    Q_PROPERTY(RecordingModes supportedRecordingModes READ supportedRecordingModes CONSTANT)
    Q_PROPERTY(Formats supportedFormats READ supportedFormats CONSTANT)
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY recordingChanged)
    Q_PROPERTY(qint64 recordedTime READ recordedTime NOTIFY recordedTimeChanged)

public:
    explicit VideoPlatform(QObject *parent = nullptr);
    ~VideoPlatform() override = default;

    enum RecordingMode : char {
        Screen = 0b001, //< records a specific output, provided its QScreen::name()
        Window = 0b010, //< records a specific window, provided its uuid
        Region = 0b100, //< records the provided region rectangle
    };
    Q_FLAG(RecordingMode)
    Q_DECLARE_FLAGS(RecordingModes, RecordingMode)
    using RecordingOption = std::variant<QScreen *, QRect, QString>;

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
     * Consider looking at guides like the following when adding new formats:
     *
     * - <https://developer.mozilla.org/en-US/docs/Web/Media/Formats/Video_codecs#recommendations_for_everyday_videos>
     */
    enum Format {
        /// A value for when no encoders are available.
        NoFormat = 0b00,
        /**
         * WebM container with the best available VP8 encoder.
         *
         * Usecase: Sharing videos over the internet.
         *
         * Advantages: Works OOTB on most Linux distros and well supported by web browsers.
         *
         * Disadvantages: Other codecs have better performance and quality.
         *
         * The reason why we are using VP8 instead of VP9 is that we had trouble getting VP9 to
         * perform well while developing KPipeWire. VP9 is also only supported by Safari since MacOS
         * Big Sur 11.3, so it may not be as good for sharing videos with Apple device users, but
         * that is a secondary concern. We should switch to using VP9 if we figure out a good way to
         * use it since it is supposed to be better than VP8.
         *
         * If we add audio support, this should use Opus.
         *
         * The flag equal to 1 should be the default format for the user settings.
         * We don't need to worry about ABI compatibility.
         */
        WebM_VP9 = 0b01,
        /**
         * MP4 container with the best available H.264 encoder.
         *
         * Usecase: When better performance and quality than VP8 is needed and sacrificing OOTB
         * Linux distro compatibility is acceptable.
         *
         * Advantages: All around good performance, quality and compatibility, even if it's a bit
         * old by now. H.265 is still patented and lacks support. AV1 is still too slow and
         * lacks support.
         *
         * Disadvantages: Doesn't work OOTB on all Linux distros.
         *
         * If we add audio support, this should use AAC.
         */
        MP4_H264 = 0b10,
        /// Used to define the default format for settings
        DefaultFormat = WebM_VP9,
    };
    Q_FLAG(Format)
    Q_DECLARE_FLAGS(Formats, Format)

    virtual RecordingModes supportedRecordingModes() const = 0;

    virtual Formats supportedFormats() const = 0;

    static QString extensionForFormat(Format format);

    bool isRecording() const;
    qint64 recordedTime() const;

protected:
    void setRecording(bool recording);
    void timerEvent(QTimerEvent *event) override;

public Q_SLOTS:
    virtual void startRecording(const QString &path,
                                VideoPlatform::RecordingMode recordingMode,
                                const VideoPlatform::RecordingOption &option,
                                bool includePointer) = 0;
    virtual void finishRecording() = 0;

Q_SIGNALS:
    void recordingChanged(bool isRecording);
    void recordingSaved(const QString &path);
    void recordedTimeChanged();

private:
    QElapsedTimer m_elapsedTimer;
    QBasicTimer m_basicTimer;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(VideoPlatform::RecordingModes)
Q_DECLARE_OPERATORS_FOR_FLAGS(VideoPlatform::Formats)
