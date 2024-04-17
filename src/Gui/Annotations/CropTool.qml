/* SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 * SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

Loader {
    id: root
    required property AnnotationViewport viewport
    readonly property AnnotationDocument document: viewport.document
    visible: active
    sourceComponent: Item {
        id: baseItem
        readonly property rect selectionRect: itemRectNormalized(selectionItem)
        function acceptCrop() {
            document.cropCanvas(selectionRect)
            resetItemRect(selectionItem)
        }
        function resetItemRect(item) {
            item.x = 0
            item.y = 0
            item.width = 0
            item.height = 0
        }
        function itemRect(item) {
            return Qt.rect(item.x, item.y, item.width, item.height)
        }
        function setItemRect(item, rect) {
            item.x = rect.x
            item.y = rect.y
            item.width = rect.width
            item.height = rect.height
        }
        function itemRectNormalized(item) {
            return G.rectNormalized(item.x, item.y, item.width, item.height)
        }
        function itemRectClipped(item, clipRect) {
            return G.rectClipped(itemRect(item), clipRect)
        }
        function maxRect() {
            return Qt.rect(0, 0, width, height)
        }
        opacity: selectionItem.visible
        Behavior on opacity {
            OpacityAnimator {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.OutCubic
            }
        }
        LayoutMirroring.enabled: false
        LayoutMirroring.childrenInherit: true

        HoverHandler {
            cursorShape: Qt.CrossCursor
        }
        DragHandler {
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            target: null
            dragThreshold: 0
            xAxis {
                minimum: 0
                maximum: baseItem.width
            }
            yAxis {
                minimum: 0
                maximum: baseItem.height
            }
            onActiveTranslationChanged: if (active) {
                selectionItem.x = centroid.pressPosition.x
                selectionItem.y = centroid.pressPosition.y
                selectionItem.width = activeTranslation.x / root.viewport.scale
                selectionItem.height = activeTranslation.y / root.viewport.scale
                setItemRect(selectionItem, itemRectClipped(selectionItem, maxRect()))
            }
            onActiveChanged: if (!active) {
                setItemRect(selectionItem, selectionRect)
            }
        }
        PointHandler {
            id: pointHandler
            acceptedButtons: Qt.AllButtons
            onActiveChanged: if (active) {
                baseItem.forceActiveFocus(Qt.MouseFocusReason)
            }
        }
        TapHandler {
            acceptedButtons: Qt.RightButton
            onSingleTapped: (eventPoint, button) => {
                resetItemRect(selectionItem)
            }
        }
        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                acceptCrop()
                event.accepted = true
            } else if (event.matches(StandardKey.Cancel)) {
                resetItemRect(selectionItem)
                event.accepted = true
            }
        }

        Connections {
            target: root.document
            // Clip selection in case someone undos/redos a crop while this tool is active
            function onUndoStackDepthChanged() {
                baseItem.setItemRect(selectionItem, baseItem.itemRectClipped(selectionItem, baseItem.maxRect()))
            }
        }

        component Overlay: Rectangle {
            color: Settings.useLightMaskColor ? "white" : "black"
            opacity: 0.5
        }
        Overlay { // top / full overlay when nothing selected
            id: topOverlay
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: selectionItem.height < 0 ? selectionItem.bottom : selectionItem.top
        }
        Overlay { // bottom
            id: bottomOverlay
            anchors.left: parent.left
            anchors.top: selectionItem.height < 0 ? selectionItem.top : selectionItem.bottom
            anchors.right: parent.right
            anchors.bottom: parent.bottom
        }
        Overlay { // left
            anchors {
                left: topOverlay.left
                top: topOverlay.bottom
                right: selectionItem.width < 0 ? selectionItem.right : selectionItem.left
                bottom: bottomOverlay.top
            }
        }
        Overlay { // right
            anchors {
                left: selectionItem.width < 0 ? selectionItem.left : selectionItem.right
                top: topOverlay.bottom
                right: topOverlay.right
                bottom: bottomOverlay.top
            }
        }

        Item { // Can have negative geometry, so we don't put visuals or handlers in here
            id: selectionItem
            visible: width !== 0 || height !== 0
        }

        Outline {
            x: selectionRect.x - effectiveStrokeWidth
            y: selectionRect.y - effectiveStrokeWidth
            width: selectionRect.width + effectiveStrokeWidth * 2
            height: selectionRect.height + effectiveStrokeWidth * 2
            zoom: root.viewport.scale
            strokeColor1: if (enabled) {
                return palette.active.highlight
            } else if (Settings.useLightMaskColor) {
                return "black"
            } else {
                return "white"
            }
            strokeColor2: "transparent"
            HoverHandler {
                cursorShape: Qt.SizeAllCursor
            }
            DragHandler {
                id: selectionDragHandler
                target: selectionItem
                dragThreshold: 0
                xAxis {
                    minimum: 0
                    maximum: baseItem.width - selectionRect.width
                }
                yAxis {
                    minimum: 0
                    maximum: baseItem.height - selectionRect.height
                }
            }
            TapHandler {
                acceptedButtons: Qt.LeftButton
                onDoubleTapped: (eventPoint, button) => {
                    acceptCrop()
                }
            }
        }

        component ResizeHandle: Handle {
            id: handle
            x: selectionRect.x + relativeXForEdges(selectionRect, edges)
                + xOffsetForEdges(strokeWidth / 2, edges)
            y: selectionRect.y + relativeYForEdges(selectionRect, edges)
                + yOffsetForEdges(strokeWidth / 2, edges)
            visible: selectionItem.visible

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
                target: null
                margin: Math.min(selectionRect.width, selectionRect.height) < 12 ? 0 : 4
                dragThreshold: 0
                xAxis {
                    enabled: handle.edges & (Qt.LeftEdge | Qt.RightEdge)
                    minimum: -handle.width / 2
                    maximum: baseItem.width - handle.width / 2
                }
                yAxis {
                    enabled: handle.edges & (Qt.TopEdge | Qt.BottomEdge)
                    minimum: -handle.height / 2
                    maximum: baseItem.height - handle.height / 2
                }
                onTranslationChanged: (delta) => {
                    if (active && (delta.x !== 0 || delta.y !== 0)) {
                        delta.x /= root.viewport.scale
                        delta.y /= root.viewport.scale
                        if (handle.edges & Qt.LeftEdge) {
                            selectionItem.width -= delta.x
                            selectionItem.x += delta.x
                        } else if (handle.edges & Qt.RightEdge) {
                            selectionItem.width += delta.x
                        }
                        if (handle.edges & Qt.TopEdge) {
                            selectionItem.height -= delta.y
                            selectionItem.y += delta.y
                        } else if (handle.edges & Qt.BottomEdge) {
                            selectionItem.height += delta.y
                        }
                        setItemRect(selectionItem, itemRectClipped(selectionItem, maxRect()))
                    }
                }
                onActiveChanged: if (!active) {
                    setItemRect(selectionItem, selectionRect)
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
    }
}
