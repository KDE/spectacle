/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Shapes
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

Shape {
    id: root
    // usually the scale will be set elsewhere, so set this to what the scale would be
    property real zoom: 1
    property real strokeWidth: 1
    // dash color 1
    property color strokeColor1: palette.highlight
    // dash color 2
    property color strokeColor2: palette.base

    property alias svgPath: pathSvg.path

    property alias pathScale: shapePath.scale
    property alias pathHints: shapePath.pathHints

    readonly property alias effectiveStrokeWidth: shapePath.strokeWidth
    readonly property alias startX: shapePath.startX
    readonly property alias startY: shapePath.startY
    // Get a rectangular SVG path
    function rectanglePath(x, y, w, h) {
        // absolute start at top-left,
        // relative line to top-right,
        // relative line to bottom-right
        // relative line to bottom-left
        // close path (automatic line to top-left)
        return `M ${x},${y}
                l ${w},0
                l 0,${h}
                l ${-w},0
                z`
    }

    preferredRendererType: Shape.CurveRenderer


    ShapePath {
        id: shapePath
        fillColor: "transparent"
        // ensure outline is always thick enough to be visible, but grows with zoom
        strokeWidth: Math.max(root.strokeWidth / root.zoom, 1 / Screen.devicePixelRatio)
        strokeColor: root.strokeColor1
        // Solid line because it's easier to do the alternating color effect this way.
        strokeStyle: ShapePath.SolidLine
        joinStyle: ShapePath.MiterJoin
        startX: strokeWidth / 2
        startY: startX
        PathSvg {
            id: pathSvg
            path: rectanglePath(strokeWidth / 2, strokeWidth / 2,
                                width - strokeWidth, height - strokeWidth)
        }
    }
}
