/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import org.kde.spectacle.private

AnnotationViewport {
    id: root

    document: SpectacleCore.annotationDocument
    viewportRect: Qt.rect(0, 0, width, height)

    onPressedChanged: if (pressed) {
        if (textTool.shouldShow) {
            textTool.forceActiveFocus(Qt.MouseFocusReason);
        }
    }

    Item {
        x: -root.viewportRect.x
        y: -root.viewportRect.y
        transformOrigin: Item.TopLeft
        TextTool {
            id: textTool
            viewport: root
        }
        SelectionTool {
            id: selectionTool
            viewport: root
        }
        HoverOutline {
            viewport: root
            hidden: selectionTool.hovered || selectionTool.dragging
        }
    }

    Shortcut {
        enabled: root.enabled
        sequences: [StandardKey.Undo]
        onActivated: root.document.undo()
    }
    Shortcut {
        enabled: root.enabled
        sequences: [StandardKey.Redo]
        onActivated: root.document.redo()
    }
}
