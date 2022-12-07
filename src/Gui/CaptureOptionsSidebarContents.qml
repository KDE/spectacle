/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQml 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Templates 2.15 as T
import org.kde.kirigami 2.19 as Kirigami
import org.kde.spectacle.private 1.0
import "Annotations"

ColumnLayout {
    id: root

    component ToolButton: QQC2.ToolButton {
        Layout.fillHeight: true
        width: display === QQC2.ToolButton.IconOnly ? height : implicitWidth
        focusPolicy: Qt.StrongFocus
        display: QQC2.AbstractButton.TextBesideIcon
        QQC2.ToolTip.text: text
        QQC2.ToolTip.visible: (hovered || pressed) && display === QQC2.ToolButton.IconOnly
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
    }



    Kirigami.Heading {
        text: i18n("Capture Mode")
        level: 3
    }
    Repeater {
        model: SpectacleCore.captureModeModel
        QQC2.RadioButton {
            text: model.display
            autoExclusive: true
            checkable: true
            checked: Settings.captureMode === model.captureMode
            onToggled: {
                if (!checked) {
                    return;
                }
                Settings.captureMode = model.captureMode;
            }
        }
    }


    Kirigami.Heading {
        Layout.topMargin: Kirigami.Units.largeSpacing * 2
        text: i18n("Options")
        level: 3
    }
    QQC2.CheckBox {
        text: i18n("Include mouse pointer")
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        QQC2.ToolTip.visible: hovered
        QQC2.ToolTip.text: i18n("Show the mouse cursor in the screenshot image")
        checked: Settings.includePointer
        onToggled: Settings.includePointer = checked;
    }
    QQC2.CheckBox {
        text: i18n("Include window titlebar and borders")
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        QQC2.ToolTip.visible: hovered
        QQC2.ToolTip.text: i18n("Show the window title bar, the minimize/maximize/close buttons, and the window border")
        checked: Settings.includeDecorations
        onToggled: Settings.includeDecorations = checked
    }
    QQC2.CheckBox {
        text: i18n("Capture the current pop-up only")
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        QQC2.ToolTip.visible: hovered
        QQC2.ToolTip.text: i18n("Capture only the current pop-up window (like a menu, tooltip etc).\nIf disabled, the pop-up is captured along with the parent window")
        checked: Settings.transientOnly
        onToggled: Settings.transientOnly = checked
    }
    QQC2.CheckBox {
        text: i18n("Quit after manual Save or Copy")
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        QQC2.ToolTip.visible: hovered
        QQC2.ToolTip.text: i18n("Quit Spectacle after manually saving or copying the image")
        checked: Settings.quitAfterSaveCopyExport
        onToggled: Settings.quitAfterSaveCopyExport = checked
    }

    RowLayout {
        QQC2.Label {
            text: i18n("Delay:")
        }
        DelaySpinBox {}
        QQC2.CheckBox {
            text: i18n("On Click")
            checked: Settings.captureOnClick
            onToggled: Settings.captureOnClick = checked
        }
    }

    Item {
        Layout.fillHeight: true
    }

    QQC2.Button {
        Layout.alignment: Qt.AlignCenter
        // Can't rely on checked since clicking also toggles checked
        readonly property bool showCancel: SpectacleCore.captureTimeRemaining > 0

        function cancelText(seconds) {
            return i18np("Cancel (%1 second)", "Cancel (%1 seconds)", Math.ceil(seconds))
        }

        checked: showCancel
        width: if (showCancel) {
            return root.getButtonWidth(display, cancelText(Settings.captureDelay), icon.name)
        } else {
            return display === QQC2.ToolButton.IconOnly ? height : implicitWidth
        }
        icon.name: showCancel ? "dialog-cancel" : "list-add"
        text: showCancel ?
            cancelText(SpectacleCore.captureTimeRemaining / 1000)
            : i18n("Take a New Screenshot")
        onClicked: if (showCancel) {
            SpectacleCore.cancelScreenshot()
        } else {
            SpectacleCore.takeNewScreenshot()
        }
    }
    Item {
        Layout.fillHeight: true
    }

}
