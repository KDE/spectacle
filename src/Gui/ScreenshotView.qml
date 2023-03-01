/*
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQml 2.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.20 as Kirigami
import org.kde.spectacle.private 1.0

import "Annotations"

// Can't use ScrollView because ScrollView prevents mice from being able to drag the view
EmptyPage {
    id: root

    readonly property real minZoom: Math.min(flickable.width / annotationEditor.implicitWidth,
                                            flickable.height / annotationEditor.implicitHeight)
    readonly property real maxZoom: Math.max(minZoom, 8)
    readonly property real defaultZoom: Math.min(
        // sometimes flickable's contentWidth/contentHeight can return -1 after setting them to -1,
        // but not always, so get content size from the flickable's contentItem directly.
        flickable.contentItem.width / annotationEditor.implicitWidth,
        flickable.contentItem.height / annotationEditor.implicitHeight
    )
    readonly property real effectiveZoom: annotationEditor.effectiveZoom

    function zoomByFactor(factor, centerPos = flickable.mapToItem(flickable.contentItem,
                                                                  flickable.width / 2,
                                                                  flickable.height / 2)) {
        let newWidth = Math.max(flickable.width, // min
                       Math.min(annotationEditor.width * factor,
                                annotationEditor.implicitWidth * maxZoom)) // max
        let newHeight = Math.max(flickable.height, // min
                        Math.min(annotationEditor.height * factor,
                                 annotationEditor.implicitHeight * maxZoom)) // max
        flickable.resizeContent(newWidth, newHeight, centerPos)
        flickable.returnToBounds()
        if (newWidth === flickable.width) {
            flickable.contentWidth = -1 // reset to follow flickable width
        }
        if (newHeight === flickable.height) {
            flickable.contentHeight = -1 // reset to follow flickable height
        }
    }

    function zoomToPercent(percent, centerPos = flickable.mapToItem(flickable.contentItem,
                                                                    flickable.width / 2,
                                                                    flickable.height / 2)) {
        let newWidth = Math.max(flickable.width, // min
                       Math.min(annotationEditor.implicitWidth * percent,
                                annotationEditor.implicitWidth * maxZoom)) // max
        let newHeight = Math.max(flickable.height, // min
                        Math.min(annotationEditor.implicitHeight * percent,
                                 annotationEditor.implicitHeight * maxZoom)) // max
        flickable.resizeContent(newWidth, newHeight, centerPos)
        flickable.returnToBounds()
        if (newWidth === flickable.width) {
            flickable.contentWidth = -1 // reset to follow flickable width
        }
        if (newHeight === flickable.height) {
            flickable.contentHeight = -1 // reset to follow flickable height
        }
    }

    leftPadding: mirrored && verticalScrollBar.visible ? verticalScrollBar.width : 0
    rightPadding: !mirrored && verticalScrollBar.visible ? verticalScrollBar.width : 0
    bottomPadding: horizontalScrollBar.visible ? horizontalScrollBar.height : 0

    contentItem: Flickable {
        id: flickable

        clip: contextWindow.annotating
        interactive: contextWindow.annotating
            && AnnotationDocument.tool.type === AnnotationDocument.None
        boundsBehavior: Flickable.StopAtBounds
        rebound: Transition {} // Instant transition. Null doesn't do this.

        // Needed to re-center the content when the window has been
        // resized to be larger than the content after zooming in.
        // Can't use contentWidth, contentHeight, contentX and contentY here for some reason.
        // Doing so causes the content to be positioned wrong or make the centering not work.
        Binding on contentItem.x {
            value: contextWindow.dprRound((flickable.width - flickable.contentItem.width) / 2)
            when: flickable.width > flickable.contentItem.width
            restoreMode: Binding.RestoreBindingOrValue
        }
        Binding on contentItem.y {
            value: contextWindow.dprRound((flickable.height - flickable.contentItem.height) / 2)
            when: flickable.height > flickable.contentItem.height
            restoreMode: Binding.RestoreBindingOrValue
        }

        Kirigami.WheelHandler {
            property point angleDelta: Qt.point(0,0)
            Binding on angleDelta { // reset when annotation tools are hidden
                value: Qt.point(0,0)
                when: !contextWindow.annotating
                restoreMode: Binding.RestoreNone
            }
            target: flickable
            keyNavigationEnabled: true
            scrollFlickableTarget: contextWindow.annotating
            onWheel: if (wheel.modifiers & Qt.ControlModifier && scrollFlickableTarget) {
                // apparently it's impossible to add points to each other directly in QML
                angleDelta.x += wheel.angleDelta.x
                angleDelta.y += wheel.angleDelta.y
                if (angleDelta.x >= 120 || angleDelta.y >= 120) {
                    angleDelta = Qt.point(0,0)
                    const centerPos = flickable.mapToItem(flickable.contentItem, wheel.x, wheel.y)
                    root.zoomByFactor(1.25, centerPos)
                } else if (angleDelta.x <= -120 || angleDelta.y <= -120) {
                    angleDelta = Qt.point(0,0)
                    const centerPos = flickable.mapToItem(flickable.contentItem, wheel.x, wheel.y)
                    root.zoomByFactor(0.8, centerPos)
                }
                wheel.accepted = true
            }
        }

        QQC2.ScrollBar.vertical: QQC2.ScrollBar {
            id: verticalScrollBar
            parent: root
            z: 1
            anchors.right: parent.right
            y: root.topPadding
            height: root.availableHeight
            active: horizontalScrollBar.active
        }
        QQC2.ScrollBar.horizontal: QQC2.ScrollBar {
            id: horizontalScrollBar
            parent: root
            z: 1
            x: root.leftPadding
            anchors.bottom: parent.bottom
            width: root.availableWidth
            active: verticalScrollBar.active
        }

        MouseArea {
            anchors.fill: parent
            z: -1
            enabled: flickable.interactive
                && (flickable.contentItem.width > flickable.width
                    || flickable.contentItem.height > flickable.height)
            cursorShape: enabled ?
                (pressed || flickable.dragging ? Qt.ClosedHandCursor : Qt.OpenHandCursor)
                : undefined
        }

        MouseArea {
            anchors.fill: annotationEditor
            cursorShape: enabled ?
                (pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor)
                : undefined
            enabled: !contextWindow.annotating
            onPositionChanged: {
                contextWindow.startDrag()
            }
        }

        AnnotationEditor {
            id: annotationEditor
            x: contextWindow.dprRound((parent.width - width) / 2)
            y: contextWindow.dprRound((parent.height - height) / 2)
            implicitWidth: contextWindow.imageSize.width / contextWindow.imageDpr
            implicitHeight: contextWindow.imageSize.height / contextWindow.imageDpr
            transformOrigin: Item.TopLeft
            zoom: Math.min(1, Math.max(root.minZoom, root.defaultZoom))
            scale: Math.max(1, Math.min(root.maxZoom, root.defaultZoom))
            antialiasing: false
            smooth: !contextWindow.annotating || effectiveZoom <= 2
            width: implicitWidth * (zoom < 1 ? zoom : scale)
            height: implicitHeight * (zoom < 1 ? zoom : scale)
            smooth: !contextWindow.annotating || effectiveZoom < 2
            visible: true
            enabled: contextWindow.annotating
                && AnnotationDocument.tool.type !== AnnotationDocument.None
        }
    }

    state: "normal"
    states: [
        State {
            name: "annotating"
            when: contextWindow.annotating
            PropertyChanges {
                target: flickable
                contentWidth: annotationEditor.implicitWidth
                contentHeight: annotationEditor.implicitHeight
            }
        },
        State {
            name: "normal"
            when: !contextWindow.annotating
            PropertyChanges {
                target: flickable
                contentWidth: -1
                contentHeight: -1
            }
        }
    ]
    transitions: [
        Transition {
            NumberAnimation {
                target: flickable
                properties: "contentWidth,contentHeight"
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutSine
            }
        }
    ]
}
