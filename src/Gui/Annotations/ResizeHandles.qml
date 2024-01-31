/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private
import ".."

AnimatedLoader {
    id: root
    required property AnnotationViewport viewport
    readonly property AnnotationDocument document: viewport.document
    readonly property bool shouldShow: enabled
        && document.tool.type === AnnotationTool.SelectTool
        && document.selectedItem.hasSelection
        && (document.selectedItem.options & AnnotationTool.TextOption) === 0
    property bool dragging: false

    state: shouldShow ? "active" : "inactive"

    sourceComponent: Item {
        id: resizeHandles
        readonly property bool dragging: tlHandle.active || tHandle.active || trHandle.active
                                    || lHandle.active || rHandle.active
                                    || blHandle.active || bHandle.active || brHandle.active

        LayoutMirroring.enabled: false
        LayoutMirroring.childrenInherit: true

        focus: true

        Binding {
            target: root
            property: "dragging"
            value: resizeHandles.dragging
            when: root.shouldShow
            restoreMode: Binding.RestoreNone
        }

        // These have to be set here to avoid having a (0,0,0,0) rect.
        Binding {
            target: root
            property: "x"
            value: root.document.selectedItem.mousePath.boundingRect.x
            when: root.shouldShow
            restoreMode: Binding.RestoreNone
        }
        Binding {
            target: root
            property: "y"
            value: root.document.selectedItem.mousePath.boundingRect.y
            when: root.shouldShow
            restoreMode: Binding.RestoreNone
        }
        Binding {
            target: root
            property: "width"
            value: root.document.selectedItem.mousePath.boundingRect.width
            when: root.shouldShow
            restoreMode: Binding.RestoreNone
        }
        Binding {
            target: root
            property: "height"
            value: root.document.selectedItem.mousePath.boundingRect.height
            when: root.shouldShow
            restoreMode: Binding.RestoreNone
        }

        SelectionBackground {
            id: outline
            svgPath: root.document.selectedItem.mousePath.svgPath
            zoom: root.viewport.effectiveZoom
            pathScale: Qt.size((root.width + effectiveStrokeWidth) / root.width,
                               (root.height + effectiveStrokeWidth) / root.height)
            x: -startX - boundingRect.x
            y: -startY - boundingRect.y
            width: boundingRect.width
            height: boundingRect.height
            containsMode: SelectionBackground.FillContains
            HoverHandler {
                cursorShape: Qt.SizeAllCursor
            }
        }

        component ResizeHandle: Handle {
            id: handle
            property int edges
            readonly property alias active: dragHandler.active

            implicitWidth: Kirigami.Units.gridUnit + Kirigami.Units.gridUnit % 2
            implicitHeight: Kirigami.Units.gridUnit + Kirigami.Units.gridUnit % 2
            visible: root.document.selectedItem.hasSelection
                && (root.document.selectedItem.options & AnnotationTool.NumberOption) === 0
            enabled: visible

            HoverHandler {
                cursorShape: {
                    if (enabled) {
                        if (handle.edges === (Qt.LeftEdge | Qt.TopEdge)
                            || handle.edges === (Qt.RightEdge | Qt.BottomEdge)) {
                            return Qt.SizeFDiagCursor;
                        } else if (handle.edges === Qt.LeftEdge
                            || handle.edges === Qt.RightEdge) {
                            return Qt.SizeHorCursor;
                        } else if (handle.edges === (Qt.LeftEdge | Qt.BottomEdge)
                            || handle.edges === (Qt.RightEdge | Qt.TopEdge)) {
                            return Qt.SizeBDiagCursor;
                        } else if (handle.edges === Qt.TopEdge
                            || handle.edges === Qt.BottomEdge) {
                            return Qt.SizeVerCursor;
                        }
                    } else {
                        return undefined
                    }
                }
            }
            DragHandler {
                id: dragHandler
                target: null
                dragThreshold: 0
                onActiveTranslationChanged: if (active) {
                    let dx = activeTranslation.x / viewport.effectiveZoom
                    let dy = activeTranslation.y / viewport.effectiveZoom
                    root.document.selectedItem.transform(dx, dy, edges)
                }
                onActiveChanged: if (!active) {
                    root.document.selectedItem.commitChanges()
                }
            }
        }
        ResizeHandle {
            id: tlHandle
            anchors {
                horizontalCenter: parent.left
                verticalCenter: parent.top
            }
            startAngle: 90
            sweepAngle: 270
            edges: Qt.TopEdge | Qt.LeftEdge
        }
        ResizeHandle {
            id: lHandle
            anchors {
                horizontalCenter: parent.left
                verticalCenter: parent.verticalCenter
            }
            startAngle: 90
            sweepAngle: 180
            edges: Qt.LeftEdge
        }
        ResizeHandle {
            id: blHandle
            anchors {
                horizontalCenter: parent.left
                verticalCenter: parent.bottom
            }
            startAngle: 0
            sweepAngle: 270
            edges: Qt.BottomEdge | Qt.LeftEdge
        }
        ResizeHandle {
            id: tHandle
            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.top
            }
            startAngle: 180
            sweepAngle: 180
            edges: Qt.TopEdge
        }
        ResizeHandle {
            id: bHandle
            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.bottom
            }
            startAngle: 0
            sweepAngle: 180
            edges: Qt.BottomEdge
        }
        ResizeHandle {
            id: trHandle
            anchors {
                horizontalCenter: parent.right
                verticalCenter: parent.top
            }
            startAngle: 180
            sweepAngle: 270
            edges: Qt.TopEdge | Qt.RightEdge
        }
        ResizeHandle {
            id: rHandle
            anchors {
                horizontalCenter: parent.right
                verticalCenter: parent.verticalCenter
            }
            startAngle: 270
            sweepAngle: 180
            edges: Qt.RightEdge
        }
        ResizeHandle {
            id: brHandle
            anchors {
                horizontalCenter: parent.right
                verticalCenter: parent.bottom
            }
            startAngle: 270
            sweepAngle: 270
            edges: Qt.BottomEdge | Qt.RightEdge
        }
        Component.onCompleted: forceActiveFocus()
    }
}


