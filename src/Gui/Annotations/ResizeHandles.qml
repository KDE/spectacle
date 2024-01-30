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

        component Handle: Rectangle {
            id: handle
            property int edges
            readonly property int effectiveEdges: {
                const invertedW = root.width < 0;
                const invertedH = root.height < 0;
                let ret = 0;
                if ((edges & Qt.LeftEdge && !invertedW) || (edges & Qt.RightEdge && invertedW)) {
                    ret |= Qt.LeftEdge;
                }
                if ((edges & Qt.RightEdge && !invertedW) || (edges & Qt.LeftEdge && invertedW)) {
                    ret |= Qt.RightEdge;
                }
                if ((edges & Qt.TopEdge && !invertedH) || (edges & Qt.BottomEdge && invertedH)) {
                    ret |= Qt.TopEdge;
                }
                if ((edges & Qt.BottomEdge && !invertedH) || (edges & Qt.TopEdge && invertedH)) {
                    ret |= Qt.BottomEdge;
                }
                return ret;
            }
            readonly property alias active: dragHandler.active
            implicitWidth: Kirigami.Units.gridUnit + Kirigami.Units.gridUnit % 2
            implicitHeight: Kirigami.Units.gridUnit + Kirigami.Units.gridUnit % 2
            visible: root.document.selectedItem.hasSelection
                && (root.document.selectedItem.options & AnnotationTool.NumberOption) === 0
            enabled: visible
            color: Kirigami.Theme.highlightColor
            radius: height / 2

            HoverHandler {
                cursorShape: {
                    if (enabled) {
                        if (handle.effectiveEdges === (Qt.LeftEdge | Qt.TopEdge)
                            || handle.effectiveEdges === (Qt.RightEdge | Qt.BottomEdge)) {
                            return Qt.SizeFDiagCursor;
                        } else if (handle.effectiveEdges === Qt.LeftEdge
                            || handle.effectiveEdges === Qt.RightEdge) {
                            return Qt.SizeHorCursor;
                        } else if (handle.effectiveEdges === (Qt.LeftEdge | Qt.BottomEdge)
                            || handle.effectiveEdges === (Qt.RightEdge | Qt.TopEdge)) {
                            return Qt.SizeBDiagCursor;
                        } else if (handle.effectiveEdges === Qt.TopEdge
                            || handle.effectiveEdges === Qt.BottomEdge) {
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
        Handle {
            id: tlHandle
            anchors {
                horizontalCenter: parent.left
                verticalCenter: parent.top
            }
            edges: Qt.TopEdge | Qt.LeftEdge
        }
        Handle {
            id: lHandle
            anchors {
                horizontalCenter: parent.left
                verticalCenter: parent.verticalCenter
            }
            edges: Qt.LeftEdge
        }
        Handle {
            id: blHandle
            anchors {
                horizontalCenter: parent.left
                verticalCenter: parent.bottom
            }
            edges: Qt.BottomEdge | Qt.LeftEdge
        }
        Handle {
            id: tHandle
            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.top
            }
            edges: Qt.TopEdge
        }
        Handle {
            id: bHandle
            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.bottom
            }
            edges: Qt.BottomEdge
        }
        Handle {
            id: trHandle
            anchors {
                horizontalCenter: parent.right
                verticalCenter: parent.top
            }
            edges: Qt.TopEdge | Qt.RightEdge
        }
        Handle {
            id: rHandle
            anchors {
                horizontalCenter: parent.right
                verticalCenter: parent.verticalCenter
            }
            edges: Qt.RightEdge
        }
        Handle {
            id: brHandle
            anchors {
                horizontalCenter: parent.right
                verticalCenter: parent.bottom
            }
            edges: Qt.BottomEdge | Qt.RightEdge
        }
        Component.onCompleted: forceActiveFocus()
    }
}


