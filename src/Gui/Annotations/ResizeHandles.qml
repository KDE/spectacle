/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQml 2.15
import QtGraphicalEffects 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.20 as Kirigami
import org.kde.spectacle.private 1.0
import ".."

AnimatedLoader {
    id: root
    required property AnnotationViewport viewport
    readonly property AnnotationDocument document: viewport.document
    readonly property bool shouldShow: enabled
        && document.tool.type === AnnotationDocument.ChangeAction
        && document.selectedAction.type !== AnnotationDocument.None
        && document.selectedAction.type !== AnnotationDocument.Text

    state: shouldShow ? "active" : "inactive"

    sourceComponent: Item {
        id: resizeHandles
        readonly property bool pressed: tlHandle.pressed || tHandle.pressed || trHandle.pressed
                                    || lHandle.pressed || rHandle.pressed
                                    || blHandle.pressed || bHandle.pressed || brHandle.pressed
        readonly property rect normalizedRect: Qt.rect(Math.min(x, x + width),
                                                       Math.min(y, y + height),
                                                       Math.abs(width), Math.abs(height))

        LayoutMirroring.enabled: false
        LayoutMirroring.childrenInherit: true

        // These have to be set here to avoid having a (0,0,0,0) rect.
        Binding {
            target: root
            property: "x"
            value: root.document.selectedAction.visualGeometry.x
            when: root.shouldShow && !resizeHandles.pressed
            restoreMode: Binding.RestoreNone
        }
        Binding {
            target: root
            property: "y"
            value: root.document.selectedAction.visualGeometry.y
            when: root.shouldShow && !resizeHandles.pressed
            restoreMode: Binding.RestoreNone
        }
        Binding {
            target: root
            property: "width"
            value: root.document.selectedAction.visualGeometry.width
            when: root.shouldShow && !resizeHandles.pressed
            restoreMode: Binding.RestoreNone
        }
        Binding {
            target: root
            property: "height"
            value: root.document.selectedAction.visualGeometry.height
            when: root.shouldShow && !resizeHandles.pressed
            restoreMode: Binding.RestoreNone
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            property real lastX
            property real lastY
            cursorShape: Qt.SizeAllCursor
            onPressed: {
                var pos = mapToItem(null, mouse.x, mouse.y);
                lastX = pos.x;
                lastY = pos.y;
            }
            onPositionChanged: {
                var pos = mapToItem(null, mouse.x, mouse.y);

                root.x += (pos.x - lastX) / viewport.effectiveZoom;
                root.y += (pos.y - lastY) / viewport.effectiveZoom;
                root.document.selectedAction.visualGeometry = Qt.rect(root.x, root.y,
                                                             root.width, root.height);
                lastX = pos.x;
                lastY = pos.y;
            }
            onReleased: root.document.selectedAction.commitChanges()
        }

        SelectionBackground {
            id: background
            zoom: root.viewport.effectiveZoom
            x: resizeHandles.normalizedRect.x - strokeWidth
            y: resizeHandles.normalizedRect.y - strokeWidth
            width: resizeHandles.normalizedRect.width + strokeWidth * 2
            height: resizeHandles.normalizedRect.height + strokeWidth * 2
        }

        component Handle: MouseArea {
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
            property real lastX
            property real lastY
            implicitWidth: Kirigami.Units.gridUnit + Kirigami.Units.gridUnit % 2
            implicitHeight: Kirigami.Units.gridUnit + Kirigami.Units.gridUnit % 2
            visible: root.document.selectedAction.type !== AnnotationDocument.Number

            Rectangle {
                id: graphics
                // TODO uncomment when the opacity masked handles are fixed
                // visible: false
                anchors.fill: parent
                color: Kirigami.Theme.highlightColor
                radius: height / 2
            }

            /* // TODO figure out why this is black
            Item {
                id: maskSource
                visible: true
                anchors.fill: graphics // Has to be the same size as source
                Rectangle {
                    x: handle.effectiveEdges & Qt.LeftEdge ? parent.width - width : 0
                    y: handle.effectiveEdges & Qt.TopEdge ? parent.height - height : 0
                    width: handle.effectiveEdges === (Qt.LeftEdge | Qt.TopEdge)
                        || handle.effectiveEdges === (Qt.RightEdge | Qt.BottomEdge)
                        || handle.effectiveEdges === (Qt.LeftEdge | Qt.BottomEdge)
                        || handle.effectiveEdges === (Qt.RightEdge | Qt.TopEdge)
                        || handle.effectiveEdges === Qt.LeftEdge
                        || handle.effectiveEdges === Qt.RightEdge ?
                        parent.width / 2 : parent.width
                    height: handle.effectiveEdges === (Qt.LeftEdge | Qt.TopEdge)
                        || handle.effectiveEdges === (Qt.RightEdge | Qt.BottomEdge)
                        || handle.effectiveEdges === (Qt.LeftEdge | Qt.BottomEdge)
                        || handle.effectiveEdges === (Qt.RightEdge | Qt.TopEdge)
                        || handle.effectiveEdges === Qt.TopEdge
                        || handle.effectiveEdges === Qt.BottomEdge ?
                        parent.height / 2 : parent.height
                }
            }

            OpacityMask {
                anchors.fill: graphics
                cached: true
                invert: true
                source: graphics
                maskSource: maskSource
            }
            */

            cursorShape: {
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
            }
            onPressed: {
                var pos = mapToItem(null, mouse.x, mouse.y);
                lastX = pos.x;
                lastY = pos.y;
            }
            onPositionChanged: {
                var pos = mapToItem(null, mouse.x, mouse.y);

                if (edges & Qt.LeftEdge) {
                    root.x += (pos.x - lastX) / viewport.effectiveZoom;
                    root.width += (lastX - pos.x) / viewport.effectiveZoom;
                } else if (edges & Qt.RightEdge) {
                    root.width += (pos.x - lastX) / viewport.effectiveZoom;
                }

                if (edges & Qt.TopEdge) {
                    root.y += (pos.y - lastY) / viewport.effectiveZoom;
                    root.height += (lastY - pos.y) / viewport.effectiveZoom;
                } else if (edges & Qt.BottomEdge) {
                    root.height += (pos.y - lastY) / viewport.effectiveZoom;
                }
                root.document.selectedAction.visualGeometry = Qt.rect(root.x, root.y,
                                                                root.width, root.height);
                lastX = pos.x;
                lastY = pos.y;
            }
            onReleased: root.document.selectedAction.commitChanges()
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


