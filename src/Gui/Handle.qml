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
        strokeWidth: 1
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
