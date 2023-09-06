/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import org.kde.kirigami 2.19 as Kirigami
import org.kde.spectacle.private 1.0

FloatingBackground {
    id: root
    property string actionsColumnText: {
        let t = i18n("Take Screenshot:")
        t += '\n'
        t += '\n' + i18n("Create new selection rectangle:")
        t += '\n'
        if (!Settings.useReleaseToCapture) {
            t += '\n' + i18n("Move selection rectangle:")
            t += '\n'
            t += '\n'
            t += '\n' + i18n("Resize selection rectangle:")
            t += '\n'
            t += '\n'
            t += '\n' + i18n("Reset selection:")
        }
        t += '\n' + i18n("Cancel:")
        return t
    }
    property string shortcutsColumnText: {
        let t = Settings.useReleaseToCapture ?
            i18nc("Mouse action", "Release left-click") : i18nc("Mouse action", "Double-click")
        t += '\n' + i18nc("Keyboard action", "Enter")
        t += '\n' + i18nc("Mouse action", "Drag outside selection rectangle")
        t += '\n' + i18nc("Keyboard action", "+ Shift: Magnifier")
        if (!Settings.useReleaseToCapture) {
            t += '\n' + i18nc("Mouse action", "Drag inside selection rectangle")
            t += '\n' + i18nc("Keyboard action", "Arrow keys")
            t += '\n' + i18nc("Keyboard action", "+ Shift: Move in 1 pixel steps")
            t += '\n' + i18nc("Mouse action", "Drag handles")
            t += '\n' + i18nc("Keyboard action", "Arrow keys + Alt")
            t += '\n' + i18nc("Keyboard action", "+ Shift: Resize in 1 pixel steps")
            t += '\n' + i18nc("Mouse action", "Right-click")
        }
        t += '\n' + i18nc("Keyboard action", "Escape")
        return t
    }
    implicitWidth: Math.round(actionsColumnLabel.implicitWidth) + actionsColumnLabel.anchors.leftMargin
        + shortcutsColumnLabel.anchors.leftMargin // spacing
        + Math.round(shortcutsColumnLabel.implicitWidth) + shortcutsColumnLabel.anchors.rightMargin
    implicitHeight: Math.round(shortcutsColumnLabel.implicitHeight)
        + shortcutsColumnLabel.anchors.topMargin
        + shortcutsColumnLabel.anchors.bottomMargin
    radius: Kirigami.Units.mediumSpacing / 2 + border.width
    corners.bottomLeftRadius: 0
    corners.bottomRightRadius: 0
    color: Qt.rgba(actionsColumnLabel.palette.window.r,
                   actionsColumnLabel.palette.window.g,
                   actionsColumnLabel.palette.window.b, 0.85)
    border.color: Qt.rgba(actionsColumnLabel.color.r,
                          actionsColumnLabel.color.g,
                          actionsColumnLabel.color.b, 0.2)
    border.width: contextWindow.dprRound(1)

    T.Label {
        id: actionsColumnLabel
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: Kirigami.Units.mediumSpacing * 2
        anchors.topMargin: anchors.leftMargin - QmlUtils.fontMetrics.descent
        anchors.bottomMargin: anchors.topMargin
        color: palette.windowText
        text: root.actionsColumnText
        textFormat: Text.PlainText
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignTop
        elide: Text.ElideNone
        wrapMode: Text.NoWrap
    }

    T.Label {
        id: shortcutsColumnLabel
        anchors.left: actionsColumnLabel.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: Kirigami.Units.mediumSpacing
        anchors.rightMargin: Kirigami.Units.mediumSpacing * 2
        anchors.topMargin: anchors.rightMargin - QmlUtils.fontMetrics.descent
        anchors.bottomMargin: anchors.topMargin
        color: palette.windowText
        text: root.shortcutsColumnText
        textFormat: Text.PlainText
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignTop
        elide: Text.ElideNone
        wrapMode: Text.NoWrap
    }
}
