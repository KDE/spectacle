/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma Singleton

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC
import org.kde.spectacle.private 1.0

/**
 * A general utilities singleton for use in QML.
 */
Item {
    id: root

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    readonly property FontMetrics fontMetrics: FontMetrics {
        id: fontMetrics
        font: Qt.application.font
    }

    readonly property real iconTextButtonHeight: iconTextButton.implicitHeight

    readonly property real textOnlyButtonHeight: textOnlyButton.implicitHeight

    readonly property real iconOnlyButtonHeight: iconOnlyButton.implicitHeight

    function getButtonSize(display = QQC.AbstractButton.TextBesideIcon, text = "text",
                           iconName = "edit-copy", isButtonMenu = false) {
        let tb = toolButtonComponent.createObject(root, {
            "display": display,
            "text": text,
            "isButtonMenu": isButtonMenu,
            "iconName": iconName
        })
        const size = Qt.size(tb.implicitWidth, tb.implicitHeight)
        tb.destroy()
        return size
    }

    Component {
        id: toolButtonComponent
        QQC.ToolButton {
            required property bool isButtonMenu
            required property string iconName
            icon.name: iconName
            text: "text"
            Accessible.role: isButtonMenu ? Accessible.ButtonMenu : Accessible.Button
        }
    }

    QQC.ToolButton {
        id: iconTextButton
        display: QQC.AbstractButton.TextBesideIcon
        icon.name: "edit-copy"
        text: "text metrics"
    }

    QQC.ToolButton {
        id: textOnlyButton
        display: QQC.AbstractButton.TextOnly
        text: "text metrics"
    }

    QQC.ToolButton {
        id: iconOnlyButton
        display: QQC.AbstractButton.IconOnly
        icon.name: "edit-copy"
    }
}
