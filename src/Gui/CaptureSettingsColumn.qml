/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami
import org.kde.spectacle.private 1.0

ColumnLayout {
    Layout.minimumWidth: delayRow.implicitWidth
    spacing: Kirigami.Units.mediumSpacing
    QQC2.CheckBox {
        Layout.fillWidth: true
        text: i18n("Include mouse pointer")
        QQC2.ToolTip.text: i18n("Show the mouse cursor in the screenshot image.")
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        QQC2.ToolTip.visible: hovered
        checked: Settings.includePointer
        onToggled: Settings.includePointer = checked
    }
    QQC2.CheckBox {
        Layout.fillWidth: true
        text: i18n("Include window titlebar and borders")
        QQC2.ToolTip.text: i18n("Show the window title bar and border when taking a screenshot of a window.")
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        QQC2.ToolTip.visible: hovered
        checked: Settings.includeDecorations
        onToggled: Settings.includeDecorations = checked
    }
    QQC2.CheckBox {
        Layout.fillWidth: true
        text: i18n("Capture the current pop-up only")
        QQC2.ToolTip.text: i18n("Capture only the current pop-up window (like a menu, tooltip etc) when taking a screenshot of a window. If disabled, the pop-up is captured along with the parent window.")
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        QQC2.ToolTip.visible: hovered
        checked: Settings.transientOnly
        onToggled: Settings.transientOnly = checked
    }
    QQC2.CheckBox {
        Layout.fillWidth: true
        text: i18n("Quit after manual Save or Copy")
        QQC2.ToolTip.text: i18n("Quit Spectacle after manually saving or copying the image.")
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        QQC2.ToolTip.visible: hovered
        checked: Settings.quitAfterSaveCopyExport
        onToggled: Settings.quitAfterSaveCopyExport = checked
    }
    QQC2.CheckBox {
        id: captureOnClickCheckBox
        Layout.fillWidth: true
        text: i18n("Capture on click")
        visible: Platform.supportedShutterModes === (Platform.Immediate | Platform.OnClick)
        QQC2.ToolTip.text: i18n("Wait for a mouse click before capturing the screenshot image.")
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        QQC2.ToolTip.visible: hovered || pressed
        checked: Platform.supportedShutterModes & Platform.OnClick && Settings.captureOnClick
        onToggled: Settings.captureOnClick = checked
    }
    RowLayout {
        id: delayRow
        spacing: parent.spacing
        QQC2.Label {
            text: i18n("Delay:")
        }
        DelaySpinBox {
            enabled: !captureOnClickCheckBox.checked
        }
    }
}
