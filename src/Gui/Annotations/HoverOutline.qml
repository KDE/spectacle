/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

AnimatedLoader {
    id: root
    required property AnnotationViewport viewport
    readonly property AnnotationDocument document: viewport.document
    readonly property bool shouldShow: enabled && !viewport.hoveredMousePath.empty
        && viewport.hoveredMousePath.boundingRect !== root.document.selectedItem.mousePath.boundingRect

    // This item will be frequently activated, so only unload it when it can't be used at all.
    active: enabled
    state: shouldShow ? "active" : "inactive"

    x: viewport.hoveredMousePath.boundingRect.x
    y: viewport.hoveredMousePath.boundingRect.y
    width: viewport.hoveredMousePath.boundingRect.width
    height: viewport.hoveredMousePath.boundingRect.height

    sourceComponent: Outline {
        id: outline
        svgPath: root.viewport.hoveredMousePath.svgPath
        zoom: root.viewport.scale
        strokeColor1: palette.text
        pathScale: Qt.size((root.width + effectiveStrokeWidth) / root.width,
                           (root.height + effectiveStrokeWidth) / root.height)
        x: -startX - boundingRect.x
        y: -startY - boundingRect.y
    }
}


