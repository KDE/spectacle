/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.7 as Kirigami
import org.kde.spectacle.private 1.0

AnnotationViewport {
    id: root

    document: AnnotationDocument
    viewportRect: Qt.rect(0, 0, width, height)
    property real effectiveZoom: zoom * scale

    onPressedChanged: if (pressed) {
        if (textTool.shouldShow) {
            textTool.forceActiveFocus(Qt.MouseFocusReason);
        }
    }

    Item {
        x: -root.viewportRect.x
        y: -root.viewportRect.y
        scale: root.zoom < 1 ? root.zoom : 1
        transformOrigin: Item.TopLeft
        TextTool {
            id: textTool
            viewport: root
        }
        ResizeHandles {
            id: resizeHandles
            viewport: root
        }
    }
}
