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
    property bool hovered: false

    state: shouldShow ? "active" : "inactive"

    sourceComponent: Item {
        id: resizeHandles
        readonly property bool dragging: tlHandle.dragging || tHandle.dragging || trHandle.dragging
                                      || lHandle.dragging || rHandle.dragging
                                      || blHandle.dragging || bHandle.dragging || brHandle.dragging
        readonly property bool hovered: tlHandle.hovered || tHandle.hovered || trHandle.hovered
                                     || lHandle.hovered || rHandle.hovered
                                     || blHandle.hovered || bHandle.hovered || brHandle.hovered

        LayoutMirroring.enabled: false
        LayoutMirroring.childrenInherit: true

        focus: true

        Binding {
            target: root
            property: "dragging"
            value: resizeHandles.dragging
            when: root.shouldShow
            restoreMode: Binding.RestoreValue
        }
        Binding {
            target: root
            property: "hovered"
            value: resizeHandles.hovered
            when: root.shouldShow
            restoreMode: Binding.RestoreValue
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

        DashedOutline {
            id: outline
            svgPath: root.document.selectedItem.mousePath.svgPath
            strokeWidth: QmlUtils.clampPx(dprRound(1) / root.viewport.scale)
            width: root.width
            height: root.height
            x: -root.document.selectedItem.mousePath.boundingRect.x
            y: -root.document.selectedItem.mousePath.boundingRect.y
            pathScale: {
                const pathBounds = root.document.selectedItem.mousePath.boundingRect
                return Qt.size(outerStrokeScaleValue(pathBounds.width, outline.strokeWidth),
                               outerStrokeScaleValue(pathBounds.height, outline.strokeWidth))
            }
            transform: Translate {
                x: outline.outerStrokeTranslateValue(root.document.selectedItem.mousePath.boundingRect.x,
                                                     outline.pathScale.width, outline.strokeWidth)
                y: outline.outerStrokeTranslateValue(root.document.selectedItem.mousePath.boundingRect.y,
                                                     outline.pathScale.height, outline.strokeWidth)
            }
            containsMode: Outline.FillContains
            HoverHandler {
                cursorShape: Qt.SizeAllCursor
            }
        }

        component ResizeHandle: Handle {
            id: handle
            readonly property alias dragging: dragHandler.active
            readonly property alias hovered: hoverHandler.hovered

            // For visibility when the outline is not very rectangular
            strokeWidth: 1 / Screen.devicePixelRatio / root.viewport.scale
            x: relativeXForEdges(parent, edges)
                + xOffsetForEdges(strokeWidth, edges)
            y: relativeYForEdges(parent, edges)
                + yOffsetForEdges(strokeWidth, edges)

            visible: root.document.selectedItem.hasSelection
                && (root.document.selectedItem.options & AnnotationTool.NumberOption) === 0
            enabled: visible

            HoverHandler {
                id: hoverHandler
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


