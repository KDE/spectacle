/*
 * SPDX-FileCopyrightText: 2023 Aleix Pol i Gonzalez <aleixpol@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC
import QtMultimedia
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

FocusScope {
    id: root
    readonly property bool hasAnimatedImage: animatedImage.status === Image.Ready
    readonly property bool hasContent: hasAnimatedImage || mediaPlayer.hasVideo
    readonly property bool playing: hasAnimatedImage ? animatedImage.playing : mediaPlayer.playing

    function togglePlay() {
        if (root.playing) {
            if (root.hasAnimatedImage) {
                animatedImage.playing = false
                return
            }
            mediaPlayer.pause()
        } else {
            if (root.hasAnimatedImage) {
                animatedImage.playing = true
                return
            }
            mediaPlayer.play()
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: enabled ?
            (pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor)
            : undefined
        enabled: root.hasContent
        onPositionChanged: {
            contextWindow.startDrag()
        }
        onClicked: root.togglePlay()
    }

    // When QtMultimedia uses the FFmpeg backend, it can't play certain animated
    // image formats such as WebP.
    AnimatedImage {
        id: animatedImage
        anchors.fill: parent
        source: {
            const format = SpectacleCore.videoPlatform.formatForPath(SpectacleCore.currentVideo.toString())
            if (format === VideoPlatform.WebP || format === VideoPlatform.Gif) {
                return SpectacleCore.currentVideo
            }
            return ""
        }
        visible: status === Image.Ready
        fillMode: Image.PreserveAspectFit
    }

    VideoOutput {
        id: videoOutput
        visible: mediaPlayer.hasVideo
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectFit
    }

    MediaPlayer {
        id: mediaPlayer
        source: root.hasAnimatedImage ? "" : SpectacleCore.currentVideo
        videoOutput: videoOutput
        onHasVideoChanged: if (hasVideo) {
            pause();
        }
        onPlaybackStateChanged: if (playbackState === MediaPlayer.StoppedState) {
            pause()
        }
    }

    Kirigami.Heading {
        anchors.fill: parent
        visible: SpectacleCore.videoPlatform.isRecording
        text: i18nc("Recording in progress, %1 is the length of the recording so far", "Recording:\n%1", SpectacleCore.recordedTime)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    HoverHandler {
        id: tbHoverHandler
    }

    FloatingToolBar {
        id: toolBar
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: padding * 2
        width: root.hasAnimatedImage ? implicitWidth : parent.width - anchors.margins * 2
        visible: false
        opacity: 0
        enabled: root.hasContent
        contentItem: RowLayout {
            spacing: parent.spacing
            TtToolButton {
                id: playPauseButton
                containmentMask: Item {
                    parent: playPauseButton
                    anchors {
                        fill: parent
                        leftMargin: toolBar.mirrored ? -toolBar.rightPadding : -toolBar.leftPadding
                        rightMargin: {
                            if (root.hasAnimatedImage) {
                                return toolBar.mirrored ? -toolBar.leftPadding : -toolBar.rightPadding
                            }
                            return -toolBar.spacing / 2
                        }
                        topMargin: -toolBar.topPadding
                        bottomMargin: -toolBar.bottomPadding
                    }
                }
                icon.name: root.playing ?
                    "media-playback-pause" : "media-playback-start"
                text: root.playing ? i18n("Pause") : i18n("Play")
                display: QQC.ToolButton.IconOnly
                onClicked: root.togglePlay()
            }
            QQC.Slider {
                id: seekBar
                containmentMask: Item {
                    parent: seekBar
                    anchors {
                        fill: parent
                        leftMargin: -toolBar.spacing / 2
                        rightMargin: -toolBar.spacing / 2
                        topMargin: -toolBar.topPadding
                        bottomMargin: -toolBar.bottomPadding
                    }
                }
                visible: !root.hasAnimatedImage
                Layout.fillWidth: true
                enabled: mediaPlayer.seekable
                wheelEnabled: false
                from: 0
                to: Math.max(1, mediaPlayer.duration)
                value: mediaPlayer.position
                onMoved: {
                    const pos = Math.round(value)
                    mediaPlayer.setPosition(pos)
                }
            }
            QQC.Label {
                visible: !root.hasAnimatedImage
                leftPadding: parent.spacing
                rightPadding: parent.spacing
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideNone
                wrapMode: Text.NoWrap
                text: {
                    let position = SpectacleCore.timeFromMilliseconds(mediaPlayer.position)
                    let duration = SpectacleCore.timeFromMilliseconds(mediaPlayer.duration)
                    return position + " / " + duration
                }
            }
        }
        layer.enabled: true // makes opacity animations look better
        state: "normal"
        states: [
            State {
                name: "hovered"
                when: tbHoverHandler.hovered && root.hasContent
                PropertyChanges {
                    target: toolBar
                    opacity: 1
                }
            },
            State {
                name: "normal"
                when: !tbHoverHandler.hovered || !root.hasContent
                PropertyChanges {
                    target: toolBar
                    opacity: 0
                }
            }
        ]
        transitions: [
            Transition {
                to: "hovered"
                SequentialAnimation {
                    PropertyAction {
                        target: toolBar
                        property: "visible"
                        value: true
                    }
                    OpacityAnimator {
                        duration: Kirigami.Units.longDuration
                        easing.type: Easing.OutCubic
                    }
                }
            },
            Transition {
                to: "normal"
                SequentialAnimation {
                    OpacityAnimator {
                        duration: Kirigami.Units.longDuration
                        easing.type: Easing.OutCubic
                    }
                    PropertyAction {
                        target: toolBar
                        property: "visible"
                        value: false
                    }
                }
            }
        ]
    }
}
