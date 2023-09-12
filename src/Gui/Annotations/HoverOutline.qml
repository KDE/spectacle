/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private
import ".."

AnimatedLoader {
    id: root
    required property AnnotationViewport viewport
    readonly property AnnotationDocument document: viewport.document
    readonly property rect visualGeometry: viewport.hovered ?
        document.visualGeometryAtPoint(viewport.toDocumentPoint(viewport.hoverPosition))
        : Qt.rect(0, 0, 0, 0)

    state: enabled
        && visualGeometry.width > 0 && visualGeometry.height > 0
        && document.tool.type === AnnotationDocument.ChangeAction
        && document.selectedAction.type === AnnotationDocument.None ?
        "active" : "inactive"

    sourceComponent: SelectionBackground {
        id: outline
        zoom: root.viewport.effectiveZoom
        strokeColor1: Kirigami.Theme.textColor
        Binding on x {
            value: root.visualGeometry.x - outline.strokeWidth
            when: root.visualGeometry.width > 0
            restoreMode: Binding.RestoreNone
        }
        Binding on y {
            value: root.visualGeometry.y - outline.strokeWidth
            when: root.visualGeometry.height > 0
            restoreMode: Binding.RestoreNone
        }
        Binding on width {
            value: root.visualGeometry.width + strokeWidth * 2
            when: root.visualGeometry.width > 0
            restoreMode: Binding.RestoreNone
        }
        Binding on height {
            value: root.visualGeometry.height + strokeWidth * 2
            when: root.visualGeometry.height > 0
            restoreMode: Binding.RestoreNone
        }
    }
}


