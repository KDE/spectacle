/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Shapes
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls as QQC
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

MouseArea {
    id: root
    readonly property rect viewportRect: Geometry.mapFromPlatformRect(screenToFollow.geometry,
                                                               screenToFollow.devicePixelRatio)
    focus: true
    acceptedButtons: Qt.LeftButton | Qt.RightButton
    hoverEnabled: true
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    anchors.fill: parent
    enabled: !SpectacleCore.videoPlatform.isRecording

    component Overlay: Rectangle {
        color: Settings.useLightMaskColor ? "white" : "black"
        opacity: if (SpectacleCore.videoPlatform.isRecording) {
            return 0
        } else if (SelectionEditor.selection.empty) {
            return 0.25
        } else {
            return 0.5
        }
        LayoutMirroring.enabled: false
        Behavior on opacity {
            NumberAnimation {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.OutCubic
            }
        }
    }
    Overlay { // top / full overlay when nothing selected
        id: topOverlay
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: selectionRectangle.visible ? selectionRectangle.top : parent.bottom
    }
    Overlay { // bottom
        id: bottomOverlay
        anchors.left: parent.left
        anchors.top: selectionRectangle.visible ? selectionRectangle.bottom : undefined
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: selectionRectangle.visible && height > 0
    }
    Overlay { // left
        anchors {
            left: topOverlay.left
            top: topOverlay.bottom
            right: selectionRectangle.visible ? selectionRectangle.left : undefined
            bottom: bottomOverlay.top
        }
        visible: selectionRectangle.visible && height > 0 && width > 0
    }
    Overlay { // right
        anchors {
            left: selectionRectangle.visible ? selectionRectangle.right : undefined
            top: topOverlay.bottom
            right: topOverlay.right
            bottom: bottomOverlay.top
        }
        visible: selectionRectangle.visible && height > 0 && width > 0
    }

    DashedOutline {
        id: selectionRectangle
        readonly property real margin: strokeWidth + 1 / Screen.devicePixelRatio
        pathHints: ShapePath.PathLinear
        dashSvgPath: SpectacleCore.videoPlatform.isRecording ? svgPath : ""
        visible: !SelectionEditor.selection.empty
            && Geometry.rectIntersects(Qt.rect(x,y,width,height), Qt.rect(0,0,parent.width, parent.height))
        strokeWidth: dprRound(1)
        strokeColor: palette.active.highlight
        dashColor: SpectacleCore.videoPlatform.isRecording ? palette.active.base : strokeColor
        // We need to be a bit careful about staying out of the recorded area
        x: dprFloor(SelectionEditor.selection.x - margin - root.viewportRect.x)
        y: dprFloor(SelectionEditor.selection.y - margin - root.viewportRect.y)
        width: dprCeil(SelectionEditor.selection.right + margin - root.viewportRect.x) - x
        height: dprCeil(SelectionEditor.selection.bottom + margin - root.viewportRect.y) - y
    }

    Item {
        x: -root.viewportRect.x
        y: -root.viewportRect.y
        enabled: selectionRectangle.enabled
        visible: !SpectacleCore.videoPlatform.isRecording

        component SelectionHandle: Handle {
            id: handle
            visible: enabled && selectionRectangle.visible
                && SelectionEditor.dragLocation === SelectionEditor.None
                && Geometry.rectIntersects(Qt.rect(x,y,width,height), root.viewportRect)
            fillColor: selectionRectangle.strokeColor
            width: Kirigami.Units.gridUnit
            height: width
            transform: Translate {
                x: handle.xOffsetForEdges(selectionRectangle.strokeWidth)
                y: handle.yOffsetForEdges(selectionRectangle.strokeWidth)
            }
        }

        SelectionHandle {
            edges: Qt.TopEdge | Qt.LeftEdge
            x: dprFloor(SelectionEditor.handlesRect.x)
            y: dprFloor(SelectionEditor.handlesRect.y)
        }
        SelectionHandle {
            edges: Qt.LeftEdge
            x: dprFloor(SelectionEditor.handlesRect.x)
            y: dprRound(SelectionEditor.handlesRect.y + SelectionEditor.handlesRect.height/2 - height/2)
        }
        SelectionHandle {
            edges: Qt.LeftEdge | Qt.BottomEdge
            x: dprFloor(SelectionEditor.handlesRect.x)
            y: dprCeil(SelectionEditor.handlesRect.y + SelectionEditor.handlesRect.height - height)
        }
        SelectionHandle {
            edges: Qt.TopEdge
            x: dprRound(SelectionEditor.handlesRect.x + SelectionEditor.handlesRect.width/2 - width/2)
            y: dprFloor(SelectionEditor.handlesRect.y)
        }
        SelectionHandle {
            edges: Qt.BottomEdge
            x: dprRound(SelectionEditor.handlesRect.x + SelectionEditor.handlesRect.width/2 - width/2)
            y: dprCeil(SelectionEditor.handlesRect.y + SelectionEditor.handlesRect.height - height)
        }
        SelectionHandle {
            edges: Qt.RightEdge
            x: dprCeil(SelectionEditor.handlesRect.x + SelectionEditor.handlesRect.width - width)
            y: dprRound(SelectionEditor.handlesRect.y + SelectionEditor.handlesRect.height/2 - height/2)
        }
        SelectionHandle {
            edges: Qt.TopEdge | Qt.RightEdge
            x: dprCeil(SelectionEditor.handlesRect.x + SelectionEditor.handlesRect.width - width)
            y: dprFloor(SelectionEditor.handlesRect.y)
        }
        SelectionHandle {
            edges: Qt.RightEdge | Qt.BottomEdge
            x: dprCeil(SelectionEditor.handlesRect.x + SelectionEditor.handlesRect.width - width)
            y: dprCeil(SelectionEditor.handlesRect.y + SelectionEditor.handlesRect.height - height)
        }
    }

    Item { // separate item because it needs to be above the stuff defined above
        visible: !SpectacleCore.videoPlatform.isRecording
        width: SelectionEditor.screensRect.width
        height: SelectionEditor.screensRect.height
        x: -root.viewportRect.x
        y: -root.viewportRect.y

        // Size ToolTip
        SizeLabel {
            id: ssToolTip
            readonly property int valignment: {
                if (SelectionEditor.selection.empty) {
                    return Qt.AlignVCenter
                }
                const margin = Kirigami.Units.mediumSpacing * 2
                const w = width + margin
                const h = height + margin
                if (SelectionEditor.handlesRect.top >= h) {
                    return Qt.AlignTop
                } else if (SelectionEditor.screensRect.height - SelectionEditor.handlesRect.bottom >= h) {
                    return Qt.AlignBottom
                } else {
                    // At the bottom of the inside of the selection rect.
                    return Qt.AlignBaseline
                }
            }
            readonly property bool normallyVisible: !SelectionEditor.selection.empty
            Binding on x {
                value: contextWindow.dprRound(SelectionEditor.selection.horizontalCenter - ssToolTip.width / 2)
                when: ssToolTip.normallyVisible
                restoreMode: Binding.RestoreNone
            }
            Binding on y {
                value: {
                    let v = 0
                    if (ssToolTip.valignment & Qt.AlignBaseline) {
                        v = Math.min(SelectionEditor.selection.bottom, SelectionEditor.handlesRect.bottom - Kirigami.Units.gridUnit)
                            - ssToolTip.height - Kirigami.Units.mediumSpacing * 2
                    } else if (ssToolTip.valignment & Qt.AlignTop) {
                        v = SelectionEditor.handlesRect.top
                            - ssToolTip.height - Kirigami.Units.mediumSpacing * 2
                    } else if (ssToolTip.valignment & Qt.AlignBottom) {
                        v = SelectionEditor.handlesRect.bottom + Kirigami.Units.mediumSpacing * 2
                    } else {
                        v = (root.height - ssToolTip.height) / 2 - parent.y
                    }
                    return contextWindow.dprRound(v)
                }
                when: ssToolTip.normallyVisible
                restoreMode: Binding.RestoreNone
            }
            visible: opacity > 0
            opacity: ssToolTip.normallyVisible
                && Geometry.rectIntersects(Qt.rect(x,y,width,height), root.viewportRect)
            Behavior on opacity {
                NumberAnimation {
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.OutCubic
                }
            }
            size: Geometry.rawSize(SelectionEditor.selection.size, SelectionEditor.devicePixelRatio) // TODO: real pixel size on wayland
            padding: Kirigami.Units.mediumSpacing * 2
            topPadding: padding - QmlUtils.fontMetrics.descent
            bottomPadding: topPadding
            background: FloatingBackground {
                implicitWidth: Math.ceil(parent.contentWidth) + parent.leftPadding + parent.rightPadding
                implicitHeight: Math.ceil(parent.contentHeight) + parent.topPadding + parent.bottomPadding
                color: Qt.rgba(parent.palette.window.r,
                            parent.palette.window.g,
                            parent.palette.window.b, 0.85)
                border.color: Qt.rgba(parent.palette.windowText.r,
                                    parent.palette.windowText.g,
                                    parent.palette.windowText.b, 0.2)
                border.width: contextWindow.dprRound(1)
            }
        }
    }

    Connections {
        target: contextWindow
        function onVisibilityChanged(visibility) {
            if (visibility !== Window.Hidden && visibility !== Window.Minimized) {
                contextWindow.raise()
                if (root.containsMouse) {
                    contextWindow.requestActivate()
                }
            }
        }
    }
}
