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

    x: -root.document.canvasRect.x
    y: -root.document.canvasRect.y
    width: viewport.hoveredMousePath.boundingRect.width
    height: viewport.hoveredMousePath.boundingRect.height

    sourceComponent: DashedOutline {
        id: outline
        svgPath: root.viewport.hoveredMousePath.svgPath
        strokeWidth: QmlUtils.clampPx(dprRound(1) / root.viewport.scale)
        strokeColor: palette.text
        pathScale: {
            const pathBounds = root.viewport.hoveredMousePath.boundingRect
            return Qt.size(outerStrokeScaleValue(pathBounds.width, strokeWidth),
                           outerStrokeScaleValue(pathBounds.height, strokeWidth))
        }
        transform: Translate {
            x: outline.outerStrokeTranslateValue(root.viewport.hoveredMousePath.boundingRect.x,
                                                 outline.pathScale.width, outline.strokeWidth)
            y: outline.outerStrokeTranslateValue(root.viewport.hoveredMousePath.boundingRect.y,
                                                 outline.pathScale.height, outline.strokeWidth)
        }
    }
}


