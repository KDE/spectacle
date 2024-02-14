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

    document: AnnotationDocument
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
        ResizeHandles {
            id: resizeHandles
            viewport: root
        }
        HoverOutline {
            viewport: root
            enabled: !resizeHandles.dragging
        }
    }
}
