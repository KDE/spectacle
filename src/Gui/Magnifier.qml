/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQml 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami
import org.kde.spectacle.private 1.0

ShaderEffectSource {
    id: root
    required property AnnotationViewport viewport
    required property point targetPoint
    property int factor: 3

    implicitWidth: {
        const w = Kirigami.Units.gridUnit * 10
        return w - w % factor - factor
    }
    implicitHeight: implicitWidth
    sourceItem: viewport
    sourceRect: Qt.rect((targetPoint.x - viewport.viewportRect.x) - implicitWidth / (factor * 2),
                        (targetPoint.y - viewport.viewportRect.y) - implicitHeight / (factor * 2),
                        implicitWidth / factor, implicitHeight / factor)
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
