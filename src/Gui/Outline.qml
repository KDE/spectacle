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

    preferredRendererType: Shape.CurveRenderer

    asynchronous: true

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
            // A rectangle path
            path: `M ${shapePath.startX},${shapePath.startY}
                   L ${root.width - shapePath.startX},${shapePath.startY}
                   L ${root.width - shapePath.startX},${root.height - shapePath.startY}
                   L ${shapePath.startX},${root.height - shapePath.startY}
                   L ${shapePath.startX},${shapePath.startY}` // close path
        }
    }
}
