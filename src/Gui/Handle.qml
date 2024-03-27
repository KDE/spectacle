/* SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Shapes

Shape {
    id: root

    property alias shapePath: shapePath
    property real startAngle: 0 // Zero and 360 are 3 o'clock, positive goes clockwise
    property real sweepAngle: 360 // positive goes clockwise
    property real xOffset: 0
    property real yOffset: 0

    containsMode: Shape.FillContains
    preferredRendererType: Shape.CurveRenderer

    // Use a rounded physically even size so that straight edges don't look fuzzy
    implicitWidth: dprRoundEven(18)
    implicitHeight: implicitWidth

    // Round to a physically even size
    function dprRoundEven(value) {
        value = Math.round(value * Screen.devicePixelRatio)
        return (value - value % 2) / Screen.devicePixelRatio
    }

    function startAngleForEdges(edges) {
        if (edges === (Qt.TopEdge | Qt.LeftEdge)) {
            return 90
        } else if (edges === Qt.TopEdge) {
            return 180
        } else if (edges === (Qt.TopEdge | Qt.RightEdge)) {
            return 180
        } else if (edges === Qt.LeftEdge) {
            return 90
        } else if (edges === Qt.RightEdge) {
            return 270
        } else if (edges === (Qt.LeftEdge | Qt.BottomEdge)) {
            return 0
        } else if (edges === Qt.BottomEdge) {
            return 0
        } else if (edges === (Qt.RightEdge | Qt.BottomEdge)) {
            return 270
        }
        return 0
    }

    function sweepAngleForEdges(edges) {
        if (edges === (Qt.TopEdge | Qt.LeftEdge)
            || edges === (Qt.TopEdge | Qt.RightEdge)
            || edges === (Qt.LeftEdge | Qt.BottomEdge)
            || edges === (Qt.RightEdge | Qt.BottomEdge)) {
            return 270
        } else if (edges === Qt.TopEdge
            || edges === Qt.LeftEdge
            || edges === Qt.RightEdge
            || edges === Qt.BottomEdge) {
            return 180
        }
        return 360
    }

    function xOffsetForEdges(absOffset, edges) {
        if (edges & Qt.LeftEdge) {
            return -absOffset
        } else if (edges & Qt.RightEdge) {
            return absOffset
        }
        return 0
    }

    function yOffsetForEdges(absOffset, edges) {
        if (edges & Qt.TopEdge) {
            return -absOffset
        } else if (edges & Qt.BottomEdge) {
            return absOffset
        }
        return 0
    }

    function cursorShapeForEdges(edges) {
        if (edges === (Qt.LeftEdge | Qt.TopEdge)
            || edges === (Qt.RightEdge | Qt.BottomEdge)) {
            return Qt.SizeFDiagCursor;
        } else if (edges === Qt.LeftEdge || edges === Qt.RightEdge) {
            return Qt.SizeHorCursor;
        } else if (edges === (Qt.LeftEdge | Qt.BottomEdge)
            || edges === (Qt.RightEdge | Qt.TopEdge)) {
            return Qt.SizeBDiagCursor;
        } else if (edges === Qt.TopEdge || edges === Qt.BottomEdge) {
            return Qt.SizeVerCursor;
        }
        return undefined
    }

    function hAnchorForEdges(item, edges) {
        if (edges === Qt.TopEdge || edges === Qt.BottomEdge) {
            return item.horizontalCenter
        } else if (edges & Qt.LeftEdge) {
            return item.left
        } else if (edges & Qt.RightEdge) {
            return item.right
        }
        return undefined
    }

    function vAnchorForEdges(item, edges) {
        if (edges === Qt.LeftEdge || edges === Qt.RightEdge) {
            return item.verticalCenter
        } else if (edges & Qt.TopEdge) {
            return item.top
        } else if (edges & Qt.BottomEdge) {
            return item.bottom
        }
        return undefined
    }

    function relativeXForEdges(itemOrRect, edges) {
        if (edges === Qt.TopEdge || edges === Qt.BottomEdge) {
            return (itemOrRect.width - width) / 2
        } else if (edges & Qt.LeftEdge) {
            return -width / 2
        } else if (edges & Qt.RightEdge) {
            return itemOrRect.width - width / 2
        }
        return 0
    }

    function relativeYForEdges(itemOrRect, edges) {
        if (edges === Qt.LeftEdge || edges === Qt.RightEdge) {
            return (itemOrRect.height - height) / 2
        } else if (edges & Qt.TopEdge) {
            return -height / 2
        } else if (edges & Qt.BottomEdge) {
            return itemOrRect.height - height / 2
        }
        return 0
    }

    function pointAtAngle(degrees) {
        const radians = degrees * (Math.PI / 180)
        return Qt.point(pathAngleArc.radiusX * Math.cos(radians) + pathAngleArc.centerX,
                        pathAngleArc.radiusY * Math.sin(radians) + pathAngleArc.centerY)
    }

    function xAtAngle(degrees) {
        return pathAngleArc.radiusX * Math.cos(degrees * (Math.PI / 180)) + pathAngleArc.centerX
    }

    function yAtAngle(degrees) {
        return pathAngleArc.radiusY * Math.sin(degrees * (Math.PI / 180)) + pathAngleArc.centerY
    }

    ShapePath {
        id: shapePath
        fillColor: root.enabled ? palette.active.highlight : palette.disabled.highlight
        strokeWidth: 0
        strokeColor: root.enabled ? palette.active.highlightedText : palette.disabled.highlightedText
        strokeStyle: ShapePath.SolidLine
        joinStyle: ShapePath.MiterJoin
        capStyle: ShapePath.FlatCap
        // Keep stroke in bounds
        scale: Qt.size((root.width - strokeWidth) / root.width,
                       (root.height - strokeWidth) / root.height)
        PathAngleArc {
            id: pathAngleArc
            moveToStart: true // this path should not be affected by startX/startY
            radiusX: root.width / 2
            radiusY: root.height / 2
            // offset with stroke and prevent scale from being applied to the offset
            centerX: (shapePath.strokeWidth / 2 + root.xOffset) / shapePath.scale.width + radiusX
            centerY: (shapePath.strokeWidth / 2 + root.yOffset) / shapePath.scale.height + radiusY
            startAngle: root.startAngle // Zero is 3 o'clock, positive goes clockwise
            sweepAngle: root.sweepAngle // positive goes clockwise
        }
        PathLine {
            id: lineFromArcEnd
            x: if (pathAngleArc.sweepAngle % 360 === 0) {
                return root.xAtAngle(pathAngleArc.startAngle + pathAngleArc.sweepAngle)
            } else if (pathAngleArc.sweepAngle % 180 === 0) {
                return root.xAtAngle(pathAngleArc.startAngle)
            } else {
                return pathAngleArc.centerX
            }
            y: if (pathAngleArc.sweepAngle % 360 === 0) {
                return root.yAtAngle(pathAngleArc.startAngle + pathAngleArc.sweepAngle)
            } else if (pathAngleArc.sweepAngle % 180 === 0) {
                return root.yAtAngle(pathAngleArc.startAngle)
            } else {
                return pathAngleArc.centerY
            }
        }
        PathLine {
            id: lineFromCenter
            x: pathAngleArc.sweepAngle % 180 === 0 ?
                lineFromArcEnd.x : root.xAtAngle(pathAngleArc.startAngle)
            y: pathAngleArc.sweepAngle % 180 === 0 ?
                lineFromArcEnd.y : root.yAtAngle(pathAngleArc.startAngle)
        }
    }
}
