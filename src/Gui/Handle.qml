/* SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Shapes

Shape {
    id: root

    property real startAngle: 0 // Zero and 360 are 3 o'clock, positive goes clockwise
    property real sweepAngle: 360 // positive goes clockwise
    property color fillColor: enabled ? palette.active.highlight : palette.disabled.highlight
    property color strokeColor: enabled ? palette.active.highlightedText : palette.disabled.highlightedText
    property real strokeWidth: 0
    property int strokeStyle: ShapePath.SolidLine
    property int joinStyle: ShapePath.MiterJoin
    property int capStyle: ShapePath.SquareCap
    // Use a rounded physically even size so that straight edges don't look fuzzy
    // HACK: using visualWidth/visualHeight to work around Qt 6.7 binding loops with handling width
    // and height in this component. There seems to be some kind of bug in Qt 6.7.
    property real visualWidth: dprRoundEven(18)
    property real visualHeight: dprRoundEven(18)
    readonly property real radiusX: (visualWidth - strokeWidth) / 2
    readonly property real radiusY: (visualHeight - strokeWidth) / 2
    readonly property real centerX: visualWidth / 2
    readonly property real centerY: visualHeight / 2
    readonly property real arcStartX: xAtAngle(startAngle, radiusX, centerX)
    readonly property real arcStartY: yAtAngle(startAngle, radiusY, centerY)
    readonly property real arcEndX: xAtAngle(startAngle + sweepAngle, radiusX, centerX)
    readonly property real arcEndY: yAtAngle(startAngle + sweepAngle, radiusY, centerY)

    containsMode: Shape.FillContains
    preferredRendererType: Shape.CurveRenderer

    implicitWidth: visualWidth
    implicitHeight: visualHeight

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

    function relativeXForEdges(itemOrRect, edges) {
        if (edges === Qt.TopEdge || edges === Qt.BottomEdge) {
            return itemOrRect.width / 2 - centerX
        } else if (edges & Qt.LeftEdge) {
            return -centerX
        } else if (edges & Qt.RightEdge) {
            return itemOrRect.width - centerX
        }
        return 0
    }

    function relativeYForEdges(itemOrRect, edges) {
        if (edges === Qt.LeftEdge || edges === Qt.RightEdge) {
            return itemOrRect.height / 2 - centerY
        } else if (edges & Qt.TopEdge) {
            return -centerY
        } else if (edges & Qt.BottomEdge) {
            return itemOrRect.height - centerY
        }
        return 0
    }

    function xAtAngle(degrees, radiusX, centerX) {
        return radiusX * Math.cos(degrees * (Math.PI / 180)) + centerX
    }

    function yAtAngle(degrees, radiusY, centerY) {
        return radiusY * Math.sin(degrees * (Math.PI / 180)) + centerY
    }

    ShapePath {
        fillColor: root.fillColor
        strokeWidth: root.strokeWidth
        strokeColor: root.strokeColor
        strokeStyle: root.strokeStyle
        joinStyle: root.joinStyle
        capStyle: root.capStyle
        pathHints: (Math.abs(root.sweepAngle) === 360 || Math.abs(root.sweepAngle) <= 180
            ? ShapePath.PathConvex : ShapePath.PathSolid)
            | ShapePath.PathNonOverlappingControlPointTriangles
        PathAngleArc {
            moveToStart: true // this path should not be affected by startX/startY
            radiusX: root.radiusX
            radiusY: root.radiusY
            // offset with stroke and prevent scale from being applied to the offset
            centerX: root.centerX
            centerY: root.centerY
            startAngle: root.startAngle // Zero is 3 o'clock, positive goes clockwise
            sweepAngle: root.sweepAngle // positive goes clockwise
        }
        PathLine {
            id: lineFromArcEnd
            x: if (root.sweepAngle % 360 === 0) {
                return root.arcEndX
            } else if (root.sweepAngle % 180 === 0) {
                return root.arcStartX
            } else {
                return root.centerX
            }
            y: if (root.sweepAngle % 360 === 0) {
                return root.arcEndY
            } else if (root.sweepAngle % 180 === 0) {
                return root.arcStartY
            } else {
                return root.centerY
            }
        }
        PathLine {
            id: lineFromCenter
            x: if (root.sweepAngle % 180 === 0) {
                return root.arcEndX
            } else {
                return root.arcStartX
            }
            y: if (root.sweepAngle % 180 === 0) {
                return root.arcEndY
            } else {
                return root.arcStartY
            }
        }
    }
}
