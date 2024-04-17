/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma Singleton

import QtQuick
import QtQuick.Controls as QQC
import org.kde.spectacle.private

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

    // Get the ratio between two values.
    // If one or both values are not finite, not null, not undefined or zero, returns 0.
    function ratio(dividend, divisor) {
        return !Number.isFinite(dividend) || !Number.isFinite(divisor) || !dividend || !divisor ?
            0 : dividend / divisor
    }

    // Basically std::clamp from C++
    function clamp(value, min, max) {
        return Math.max(min, Math.min(value, max))
    }

    // Get a clamped pixel value.
    // The default minimum is 1 physical pixel with an item scale of 1.
    // The default maximum is positive infinity.
    function clampPx(value, min = 1 / Screen.devicePixelRatio, max = Number.POSITIVE_INFINITY) {
        return clamp(value, min, max)
    }

    // When scaling a set of points such as a path, all points are individually multiplied.
    // This means scaling up translates positively and scaling down translates negatively.
    // This can be used to get a translation for preventing translation from scaling.
    function unTranslateScale(oldValue, scale) {
        return oldValue - oldValue * scale
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
