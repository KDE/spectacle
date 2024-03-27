/*
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

import "Annotations"

// Can't use ScrollView because ScrollView prevents mice from being able to drag the view
EmptyPage {
    id: root

    readonly property real fitZoom: Math.min(flickable.width / AnnotationDocument.canvasRect.width,
                                             flickable.height / AnnotationDocument.canvasRect.height)
    readonly property real minZoom: Math.min(fitZoom, 1)
    readonly property real maxZoom: Math.max(minZoom, 8)
    readonly property real currentZoom: annotationEditor.scale
    property bool showCropTool: false

    function zoomToPercent(percent, center = flickable.mapToItem(flickable.contentItem,
                                                                 flickable.width / 2,
                                                                 flickable.height / 2)) {
        const oldW = AnnotationDocument.canvasRect.width * annotationEditor.scale
        const oldH = AnnotationDocument.canvasRect.height * annotationEditor.scale
        annotationEditor.scale = Math.max(root.minZoom, Math.min(percent, root.maxZoom))
        const w = AnnotationDocument.canvasRect.width * annotationEditor.scale
        const h = AnnotationDocument.canvasRect.height * annotationEditor.scale

        if (center.x !== 0) {
            const min = flickable.width - Math.max(w, flickable.contentItem.width)
            const target = dprRound(flickable.contentItem.x + center.x + (-center.x * w / oldW))
            flickable.contentX = -Math.max(min, Math.min(target, 0)) // max
        }
        if (center.y !== 0) {
            const min = flickable.height - Math.max(h, flickable.contentItem.height)
            const target = dprRound(flickable.contentItem.y + center.y + (-center.y * w / oldW))
            flickable.contentY = -Math.max(min, Math.min(target, 0)) // max
        }
        flickable.returnToBounds()
    }

    function zoomIn(centerPos = flickable.mapToItem(flickable.contentItem,
                                                    flickable.width / 2,
                                                    flickable.height / 2)) {
        let stepSize = 1
        if (currentZoom < 1) {
            stepSize = 0.25
        } else if (currentZoom < 2) {
            stepSize = 0.5
        }
        zoomToPercent(currentZoom - (currentZoom % stepSize) + stepSize, centerPos)
    }

    function zoomOut(centerPos = flickable.mapToItem(flickable.contentItem,
                                                     flickable.width / 2,
                                                     flickable.height / 2)) {
        let inverseRemainder = 1 - (currentZoom % 1)
        let stepSize = 1
        if (currentZoom <= 1) {
            stepSize = 0.25
        } else if (currentZoom <= 2) {
            stepSize = 0.5
        }
        zoomToPercent(currentZoom + (inverseRemainder % stepSize) - stepSize, centerPos)
    }

    leftPadding: mirrored && verticalScrollBar.visible ? verticalScrollBar.width : 0
    rightPadding: !mirrored && verticalScrollBar.visible ? verticalScrollBar.width : 0
    bottomPadding: horizontalScrollBar.visible ? horizontalScrollBar.height : 0

    contentItem: Flickable {
        id: flickable

        clip: contextWindow.annotating
        interactive: contextWindow.annotating
            && AnnotationDocument.tool.type === AnnotationTool.NoTool
        boundsBehavior: Flickable.StopAtBounds
        rebound: Transition {} // Instant transition. Null doesn't do this.
        contentWidth: Math.max(width, AnnotationDocument.canvasRect.width * annotationEditor.scale)
        contentHeight: Math.max(height, AnnotationDocument.canvasRect.height * annotationEditor.scale)

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
            horizontalStepSize: dprRound(Application.styleHints.wheelScrollLines * 20)
            verticalStepSize: dprRound(Application.styleHints.wheelScrollLines * 20)
            onWheel: wheel => {
                    if (wheel.modifiers & Qt.ControlModifier && scrollFlickableTarget) {
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
        }

        QQC.ScrollBar.vertical: QQC.ScrollBar {
            id: verticalScrollBar
            parent: root
            z: 1
            anchors.right: parent.right
            y: root.topPadding
            height: root.availableHeight
            active: horizontalScrollBar.active
        }
        QQC.ScrollBar.horizontal: QQC.ScrollBar {
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
            transformOrigin: annotationEditor.transformOrigin
            scale: annotationEditor.scale
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
            x: dprRound((flickable.contentItem.width - AnnotationDocument.canvasRect.width * scale) / 2)
            y: dprRound((flickable.contentItem.height - AnnotationDocument.canvasRect.height * scale) / 2)
            implicitWidth: AnnotationDocument.canvasRect.width
            implicitHeight: AnnotationDocument.canvasRect.height
            transformOrigin: Item.TopLeft
            scale: root.fitZoom
            visible: true
            enabled: contextWindow.annotating
                && AnnotationDocument.tool.type !== AnnotationTool.NoTool
            Keys.forwardTo: cropTool
            Keys.priority: Keys.AfterItem
        }

        CropTool {
            id: cropTool
            anchors.fill: annotationEditor
            transformOrigin: annotationEditor.transformOrigin
            scale: annotationEditor.scale
            viewport: annotationEditor
            active: root.showCropTool && contextWindow.annotating
        }

        AnimatedLoader {
            parent: flickable
            anchors.centerIn: parent
            state: cropTool.item && !cropTool.item.activeFocus && cropTool.item.opacity === 0 ? "active" : "inactive"
            sourceComponent: Kirigami.Heading {
                id: cropToolHelp
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: i18nc("@info crop tool explanation", "Click and drag to make a selection.\nDouble click the selection to accept and crop.\nRight click to clear the selection.")
                padding: cropToolHelpMetrics.height - cropToolHelpMetrics.descent
                leftPadding: cropToolHelpMetrics.height
                rightPadding: cropToolHelpMetrics.height
                background: FloatingBackground {
                    color: Qt.rgba(palette.window.r, palette.window.g, palette.window.b, 0.9)
                    radius: cropToolHelpMetrics.height
                }
                FontMetrics {
                    id: cropToolHelpMetrics
                    font: cropToolHelp.font
                }
            }
        }
    }

    state: "normal"
    states: [
        State {
            name: "annotating"
            when: contextWindow.annotating
            PropertyChanges {
                target: annotationEditor
                scale: 1
            }
        },
        State {
            name: "normal"
            when: !contextWindow.annotating
            PropertyChanges {
                target: annotationEditor
                scale: root.fitZoom
            }
        }
    ]
    transitions: [
        Transition {
            NumberAnimation {
                property: "scale"
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutSine
            }
        }
    ]
}
