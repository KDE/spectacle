/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

Loader {
    id: root
    required property AnnotationViewport viewport
    readonly property AnnotationDocument document: viewport.document
    // Used when we explicitly want to hide the outline
    property bool hidden: false
    // This will be frequently shown and hidden when using the selection tool
    active: visible && document.tool.type === AnnotationTool.SelectTool
    visible: enabled
    x: -root.document.canvasRect.x
    y: -root.document.canvasRect.y
    width: viewport.hoveredMousePath.boundingRect.width
    height: viewport.hoveredMousePath.boundingRect.height

    sourceComponent: DashedOutline {
        id: outline
        // Not animated because of scaling/flickering issues when the path becomes empty
        visible: !root.hidden && !viewport.hoveredMousePath.empty
            && viewport.hoveredMousePath.boundingRect !== root.document.selectedItem.mousePath.boundingRect
        // These shapes can be complex and don't need to synchronize with any other visuals,
        // so they don't need to be synchronous.
        asynchronous: true
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


