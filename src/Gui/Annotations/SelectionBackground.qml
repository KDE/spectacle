/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Shapes 1.15
import org.kde.kirigami 2.20 as Kirigami
import org.kde.spectacle.private 1.0

Shape {
    id: root
    // usually the scale will be set elsewhere, so set this to what the scale would be
    property real zoom: 1
    // ensure outline is always thick enough to be visible
    property real strokeWidth: Math.max(1 / zoom, 1)
    // dash color 1
    property color strokeColor1: Kirigami.Theme.highlightColor
    // dash color 2
    property color strokeColor2: Kirigami.Theme.backgroundColor

    Rectangle {
        id: rectangle
        z: -1
        anchors.fill: parent
        color: "transparent"
        border.color: root.strokeColor1
        border.width: root.strokeWidth
    }
    ShapePath {
        id: shapePath
        fillColor: "transparent"
        strokeWidth: root.strokeWidth
        strokeColor: root.strokeColor2
        strokeStyle: ShapePath.DashLine
        // for some reason, +2 makes the spacing and dash lengths the same, no matter what the strokeWidth is.
        dashPattern: [Kirigami.Units.smallSpacing / strokeWidth, Kirigami.Units.smallSpacing / strokeWidth + 2]
        dashOffset: 0
        startX: strokeWidth / 2
        startY: startX
        PathLine { x: root.width - shapePath.startX; y: shapePath.startY }
        PathLine { x: root.width - shapePath.startX; y: root.height - shapePath.startY }
        PathLine { x: shapePath.startX; y: root.height - shapePath.startY }
        PathLine { x: shapePath.startX; y: shapePath.startY }
    }
}
