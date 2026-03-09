/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private
import org.kde.kquickimageeditor

ShaderEffectSource {
    id: root
    required property AnnotationViewport viewport
    required property point targetPoint
    property int factor: 3

    implicitWidth: sourceRect.width * factor
    implicitHeight: sourceRect.height * factor
    sourceItem: viewport
    // We need a size that multiplies by the factor to an odd size to keep the graphics crisp.
    // The position needs an additional offset equal to half the size minus half a logical pixel.
    sourceRect: Qt.rect((targetPoint.x - viewport.viewportRect.x) - 33,
                        (targetPoint.y - viewport.viewportRect.y) - 33,
                        67, 67)
    smooth: false

    Item {
        id: center
        x: contextWindow.dprRound((parent.implicitWidth - width) / 2)
        y: contextWindow.dprRound((parent.implicitHeight - height) / 2)
        width: root.factor * 3
        height: root.factor * 3
    }

    Rectangle { // top
        anchors.top: parent.top
        anchors.bottom: center.top
        color: Kirigami.Theme.focusColor
        x: contextWindow.dprRound((parent.implicitWidth - width) / 2)
        width: root.factor
    }
    Rectangle { // bottom
        anchors.bottom: parent.bottom
        anchors.top: center.bottom
        color: Kirigami.Theme.focusColor
        x: contextWindow.dprRound((parent.implicitWidth - width) / 2)
        width: root.factor
    }
    Rectangle { // left
        anchors.left: parent.left
        anchors.right: center.left
        color: Kirigami.Theme.focusColor
        y: contextWindow.dprRound((parent.implicitHeight - height) / 2)
        height: root.factor
    }
    Rectangle { // right
        anchors.right: parent.right
        anchors.left: center.right
        color: Kirigami.Theme.focusColor
        y: contextWindow.dprRound((parent.implicitHeight - height) / 2)
        height: root.factor
    }
}
