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

    readonly property real fitZoom: Math.min(flickable.width / annotationEditor.implicitWidth,
                                             flickable.height / annotationEditor.implicitHeight)
    readonly property real minZoom: Math.min(fitZoom, 1)
    readonly property real maxZoom: Math.max(minZoom, 8)
    readonly property real effectiveZoom: annotationEditor.effectiveZoom

    // Flickable doesn't work well when making the contentItem smaller than the flickable,
    // so we have to create custom behavior for resizing content.
    function resizeContent(w, h, center) {
        const oldW = annotationEditor.width
        const oldH = annotationEditor.height
        annotationEditor.width = w
        annotationEditor.height = h

        // Setting contentX and contentY can cause jittery movement, especially when animating,
        // but not using them can cause the content to go out of bounds when zooming out.
        // Using returnToBounds() will not fix it. You must manually limit the positions.
        if (center.x !== 0) {
            const min = flickable.width - Math.max(w, flickable.contentItem.width)
            const target = contextWindow.dprRound(flickable.contentItem.x + center.x
                                                  + (-center.x * w / oldW))
            flickable.contentItem.x = Math.max(min,
                                      Math.min(target,
                                               0)) // max
        }
        if (center.y !== 0) {
            const min = flickable.height - Math.max(h, flickable.contentItem.height)
            const target = contextWindow.dprRound(flickable.contentItem.y + center.y
                                                  + (-center.y * w / oldW))
            flickable.contentItem.y = Math.max(min,
                                      Math.min(target,
                                               0)) // max
        }
    }

    function zoomToPercent(percent, centerPos = flickable.mapToItem(flickable.contentItem,
                                                                    flickable.width / 2,
                                                                    flickable.height / 2)) {
        let newWidth = Math.max(annotationEditor.implicitWidth * minZoom, // min
                       Math.min(annotationEditor.implicitWidth * percent,
                                annotationEditor.implicitWidth * maxZoom)) // max
        let newHeight = Math.max(annotationEditor.implicitHeight * minZoom, // min
                        Math.min(annotationEditor.implicitHeight * percent,
                                 annotationEditor.implicitHeight * maxZoom)) // max
        resizeContent(newWidth, newHeight, centerPos)
        flickable.returnToBounds()
    }

    function zoomIn(centerPos = flickable.mapToItem(flickable.contentItem,
                                                    flickable.width / 2,
                                                    flickable.height / 2)) {
        let stepSize = 1
        if (effectiveZoom < 1) {
            stepSize = 0.25
        } else if (effectiveZoom < 2) {
            stepSize = 0.5
        }
        zoomToPercent(effectiveZoom - (effectiveZoom % stepSize) + stepSize, centerPos)
    }

    function zoomOut(centerPos = flickable.mapToItem(flickable.contentItem,
                                                     flickable.width / 2,
                                                     flickable.height / 2)) {
        let inverseRemainder = 1 - (effectiveZoom % 1)
        let stepSize = 1
        if (effectiveZoom <= 1) {
            stepSize = 0.25
        } else if (effectiveZoom <= 2) {
            stepSize = 0.5
        }
        zoomToPercent(effectiveZoom + (inverseRemainder % stepSize) - stepSize, centerPos)
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
        contentWidth: Math.max(width, annotationEditor.width)
        contentHeight: Math.max(height, annotationEditor.height)

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
                    root.zoomIn(centerPos)
                } else if (angleDelta.x <= -120 || angleDelta.y <= -120) {
                    angleDelta = Qt.point(0,0)
                    const centerPos = flickable.mapToItem(flickable.contentItem, wheel.x, wheel.y)
                    root.zoomOut(centerPos)
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
            z: -1
            anchors.fill: annotationEditor
            enabled: !contextWindow.annotating
                || (flickable.interactive
                    && (flickable.contentItem.width > flickable.width
                        || flickable.contentItem.height > flickable.height))
            cursorShape: enabled ?
                (containsPress || flickable.dragging ? Qt.ClosedHandCursor : Qt.OpenHandCursor)
                : undefined
            onPositionChanged: if (!contextWindow.annotating) {
                contextWindow.startDrag()
            }
        }

        AnnotationEditor {
            id: annotationEditor
            readonly property real targetZoom: Math.min(width / implicitWidth,
                                                        height / implicitHeight)
            x: contextWindow.dprRound((flickable.contentItem.width - annotationEditor.width) / 2)
            y: contextWindow.dprRound((flickable.contentItem.height - annotationEditor.height) / 2)
            implicitWidth: document.canvasSize.width
            implicitHeight: document.canvasSize.height
            transformOrigin: Item.TopLeft
            zoom: Math.min(1, Math.max(root.minZoom, targetZoom))
            scale: Math.max(1, Math.min(root.maxZoom, targetZoom))
            antialiasing: false
            smooth: !contextWindow.annotating || effectiveZoom < 2
            width: implicitWidth * root.fitZoom
            height: implicitHeight * root.fitZoom
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
                target: annotationEditor
                width: annotationEditor.implicitWidth
                height: annotationEditor.implicitHeight
            }
        },
        State {
            name: "normal"
            when: !contextWindow.annotating
            PropertyChanges {
                target: annotationEditor
                width: annotationEditor.implicitWidth * root.fitZoom
                height: annotationEditor.implicitHeight * root.fitZoom
            }
        }
    ]
    transitions: [
        Transition {
            NumberAnimation {
                properties: "width,height"
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutSine
            }
        }
    ]
}
