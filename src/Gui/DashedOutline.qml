/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Shapes
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

Outline {
    id: root
    property alias dashColor: dashPath.strokeColor
    property alias dashCapStyle: dashPath.capStyle
    property alias dashJoinStyle: dashPath.joinStyle
    // dashPattern is a list of alternating dash and space lengths.
    // Length in logical pixels is length * strokeWidth,
    // so divide by strokeWidth if you want to set length in logical pixels.
    property alias dashPattern: dashPath.dashPattern
    property alias dashOffset: dashPath.dashOffset
    property alias dashSvgPath: dashPathSvg.path
    property alias dashPathScale: dashPath.scale
    property alias dashPathHints: dashPath.pathHints

    // A regular alternative pattern with a spacing in logical pixels
    function regularDashPattern(spacing, strokeWidth = root.strokeWidth) {
        return [spacing / strokeWidth, spacing / strokeWidth]
    }

    ShapePath {
        id: dashPath
        fillColor: "transparent"
        strokeWidth: root.strokeWidth
        strokeColor: palette.base
        strokeStyle: ShapePath.DashLine
        dashPattern: regularDashPattern(Kirigami.Units.mediumSpacing)
        dashOffset: 0
        // FlatCap ensures that dash and space length are equal.
        // With other cap styles, subtract strokeWidth * 2 from the logical pixel length of dashes.
        capStyle: ShapePath.FlatCap
        joinStyle: root.joinStyle
        scale: root.pathScale
        pathHints: root.pathHints
        PathSvg {
            id: dashPathSvg
            path: root.svgPath
        }
    }
}
