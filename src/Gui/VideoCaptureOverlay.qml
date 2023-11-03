/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls as QQC
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

MouseArea {
    id: root
    readonly property rect viewportRect: G.mapFromPlatformRect(screenToFollow.geometry,
                                                               screenToFollow.devicePixelRatio)
    focus: true
    acceptedButtons: Qt.LeftButton | Qt.RightButton
    hoverEnabled: true
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    anchors.fill: parent

    component Overlay: Rectangle {
        color: Settings.useLightMaskColor ? "white" : "black"
        opacity: 0.5
        LayoutMirroring.enabled: false
    }
    Overlay { // top / full overlay when nothing selected
        id: topOverlay
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: selectionRectangle.visible ? selectionRectangle.top : parent.bottom
        opacity: if (Selection.empty) {
            return 0.25
        } else {
            return 0.5
        }
        Behavior on opacity {
            NumberAnimation {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.OutCubic
            }
        }
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

    Rectangle {
        id: selectionRectangle
        color: "transparent"
        border.color: palette.highlight
        border.width: contextWindow.dprRound(1)
        visible: !Selection.empty && G.rectIntersects(Qt.rect(x,y,width,height),
                                                      Qt.rect(0,0,parent.width, parent.height))
        x: Selection.x - border.width - root.viewportRect.x
        y: Selection.y - border.width - root.viewportRect.y
        width: Selection.width + border.width * 2
        height: Selection.height + border.width * 2

        LayoutMirroring.enabled: false
        LayoutMirroring.childrenInherit: true
    }

    Item {
        x: -root.viewportRect.x
        y: -root.viewportRect.y
        enabled: selectionRectangle.enabled
        component Handle: Rectangle {
            visible: enabled && selectionRectangle.visible
                && !VideoPlatform.isRecording
                && SelectionEditor.dragLocation === SelectionEditor.None
                && G.rectIntersects(Qt.rect(x,y,width,height), root.viewportRect)
            color: palette.highlight
            width: Kirigami.Units.gridUnit
            height: width
            radius: width / 2
        }

        Handle {
            x: SelectionEditor.handlesRect.x
            y: SelectionEditor.handlesRect.y
        }
        Handle {
            x: SelectionEditor.handlesRect.x
            y: SelectionEditor.handlesRect.y + SelectionEditor.handlesRect.height/2 - height/2
        }
        Handle {
            x: SelectionEditor.handlesRect.x
            y: SelectionEditor.handlesRect.y + SelectionEditor.handlesRect.height - height
        }
        Handle {
            x: SelectionEditor.handlesRect.x + SelectionEditor.handlesRect.width/2 - width/2
            y: SelectionEditor.handlesRect.y
        }
        Handle {
            x: SelectionEditor.handlesRect.x + SelectionEditor.handlesRect.width/2 - width/2
            y: SelectionEditor.handlesRect.y + SelectionEditor.handlesRect.height - height
        }
        Handle {
            x: SelectionEditor.handlesRect.x + SelectionEditor.handlesRect.width - width
            y: SelectionEditor.handlesRect.y + SelectionEditor.handlesRect.height/2 - height/2
        }
        Handle {
            x: SelectionEditor.handlesRect.x + SelectionEditor.handlesRect.width - width
            y: SelectionEditor.handlesRect.y
        }
        Handle {
            x: SelectionEditor.handlesRect.x + SelectionEditor.handlesRect.width - width
            y: SelectionEditor.handlesRect.y + SelectionEditor.handlesRect.height - height
        }
    }

    Item { // separate item because it needs to be above the stuff defined above
        width: SelectionEditor.screensRect.width
        height: SelectionEditor.screensRect.height
        x: -root.viewportRect.x
        y: -root.viewportRect.y

        // Size ToolTip
        SizeLabel {
            id: ssToolTip
            readonly property int valignment: {
                if (Selection.empty) {
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
            readonly property bool normallyVisible: !Selection.empty
            Binding on x {
                value: contextWindow.dprRound(Selection.horizontalCenter - ssToolTip.width / 2)
                when: ssToolTip.normallyVisible
                restoreMode: Binding.RestoreNone
            }
            Binding on y {
                value: {
                    let v = 0
                    if (ssToolTip.valignment & Qt.AlignBaseline) {
                        v = Math.min(Selection.bottom, SelectionEditor.handlesRect.bottom - Kirigami.Units.gridUnit)
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
                && G.rectIntersects(Qt.rect(x,y,width,height), root.viewportRect)
            Behavior on opacity {
                NumberAnimation {
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.OutCubic
                }
            }
            size: G.rawSize(Selection.size, SelectionEditor.devicePixelRatio) // TODO: real pixel size on wayland
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

    // FIXME: This shortcut only exists here because spectacle interprets "Ctrl+Shift+,"
    // as "Ctrl+Shift+<" for some reason unless we use a QML Shortcut.
    Shortcut {
        sequences: [StandardKey.Preferences]
        onActivated: contextWindow.showPreferencesDialog()
    }
}
