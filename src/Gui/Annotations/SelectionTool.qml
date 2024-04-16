/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

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
            value: root.document.selectedItem.mousePath.boundingRect.x - root.document.canvasRect.x
            when: root.shouldShow
            restoreMode: Binding.RestoreNone
        }
        Binding {
            target: root
            property: "y"
            value: root.document.selectedItem.mousePath.boundingRect.y - root.document.canvasRect.y
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

        Outline {
            id: outline
            svgPath: root.document.selectedItem.mousePath.svgPath
            zoom: root.viewport.scale
            pathScale: Qt.size((root.width + effectiveStrokeWidth) / root.width,
                               (root.height + effectiveStrokeWidth) / root.height)
            x: -startX - boundingRect.x
            y: -startY - boundingRect.y
            width: boundingRect.width
            height: boundingRect.height
            containsMode: Outline.FillContains
            HoverHandler {
                cursorShape: Qt.SizeAllCursor
            }
        }

        component ResizeHandle: Handle {
            id: handle
            property int edges
            readonly property alias active: dragHandler.active

            // For visibility when the outline is not very rectangular
            strokeWidth: 1 / Screen.devicePixelRatio / root.viewport.scale
            startAngle: startAngleForEdges(edges)
            sweepAngle: sweepAngleForEdges(edges)
            x: relativeXForEdges(parent, edges)
                + xOffsetForEdges(strokeWidth, edges)
            y: relativeYForEdges(parent, edges)
                + yOffsetForEdges(strokeWidth, edges)

            visible: root.document.selectedItem.hasSelection
                && (root.document.selectedItem.options & AnnotationTool.NumberOption) === 0
            enabled: visible

            HoverHandler {
                margin: dragHandler.margin
                cursorShape: {
                    if (enabled) {
                        return handle.cursorShapeForEdges(handle.edges)
                    } else {
                        return undefined
                    }
                }
            }
            DragHandler {
                id: dragHandler
                margin: Math.min(root.width, root.height) < 12 ? 0 : 4
                target: null
                dragThreshold: 0
                onActiveTranslationChanged: if (active) {
                    let dx = activeTranslation.x / viewport.scale
                    let dy = activeTranslation.y / viewport.scale
                    root.document.selectedItem.transform(dx, dy, edges)
                }
                onActiveChanged: if (!active) {
                    root.document.selectedItem.commitChanges()
                }
            }
        }
        ResizeHandle {
            id: tlHandle
            edges: Qt.TopEdge | Qt.LeftEdge
        }
        ResizeHandle {
            id: tHandle
            edges: Qt.TopEdge
        }
        ResizeHandle {
            id: trHandle
            edges: Qt.TopEdge | Qt.RightEdge
        }
        ResizeHandle {
            id: lHandle
            edges: Qt.LeftEdge
        }
        ResizeHandle {
            id: rHandle
            edges: Qt.RightEdge
        }
        ResizeHandle {
            id: blHandle
            edges: Qt.BottomEdge | Qt.LeftEdge
        }
        ResizeHandle {
            id: bHandle
            edges: Qt.BottomEdge
        }
        ResizeHandle {
            id: brHandle
            edges: Qt.BottomEdge | Qt.RightEdge
        }
        Component.onCompleted: forceActiveFocus()
    }
}


