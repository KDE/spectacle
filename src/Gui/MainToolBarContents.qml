/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQml 2.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami
import org.kde.spectacle.private 1.0

ButtonGrid {
    id: root
    property size imageSize: Qt.size(0, 0)
    property bool showSizeLabel: false
    property bool showUndoRedo: false
    property bool showNewScreenshotButton: true
    property bool showOptionsMenu: true

    component ToolButton: QQC2.ToolButton {
        height: root.fullButtonHeight
        width: display === QQC2.ToolButton.IconOnly ? height : implicitWidth
        focusPolicy: root.focusPolicy
        display: root.displayMode
        QQC2.ToolTip.text: text
        QQC2.ToolTip.visible: (hovered || pressed) && display === QQC2.ToolButton.IconOnly
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
    }

    AnimatedLoader {
        id: sizeLabelLoader
        height: root.fullButtonHeight
        state: root.showSizeLabel && root.imageSize.width > 0 && root.imageSize.height > 0 ?
            "active" : "inactive"
        sourceComponent: SizeLabel {
            size: root.imageSize
            leftPadding: Kirigami.Units.mediumSpacing + fontMetrics.descent
            rightPadding: leftPadding
        }
    }

    AnimatedLoader {
        state: root.showUndoRedo ? "active" : "inactive"
        sourceComponent: UndoRedoGroup {
            animationsEnabled: root.animationsEnabled
            buttonHeight: root.fullButtonHeight
            focusPolicy: root.focusPolicy
            flow: root.flow
            spacing: root.spacing
        }
    }

    ToolButton {
        visible: !SpectacleCore.videoMode
        icon.name: "document-save"
        text: i18n("Save")
        onClicked: contextWindow.save()
    }

    ToolButton {
        visible: !SpectacleCore.videoMode
        icon.name: "document-save-as"
        text: i18n("Save As...")
        onClicked: contextWindow.saveAs()
    }

    ToolButton {
        visible: !SpectacleCore.videoMode
        icon.name: "edit-copy"
        text: i18n("Copy")
        onClicked: contextWindow.copyImage()
    }

    ToolButton {
        icon.name: "document-share"
        text: i18n("Export")
        down: pressed || contextWindow.exportMenu.visible
        Accessible.role: Accessible.ButtonMenu
        // for some reason, y has to be set to get the correct y pos, but x shouldn't be
        onPressed: contextWindow.exportMenu.popup(this)
    }

    ToolButton {
        id: annotationsButton
        icon.name: "edit-image"
        text: i18n("Show Annotation Tools")
        visible: !SpectacleCore.videoMode
        checkable: true
        checked: contextWindow.annotating
        onToggled: contextWindow.annotating = checked
    }

    ToolButton {
        // Can't rely on checked since clicking also toggles checked
        readonly property bool showCancel: SpectacleCore.captureTimeRemaining > 0

        function cancelText(seconds) {
            return i18np("Cancel (%1 second)", "Cancel (%1 seconds)", Math.ceil(seconds))
        }

        visible: root.showNewScreenshotButton
        checked: showCancel
        width: if (showCancel) {
            return root.getButtonWidth(display, cancelText(Settings.captureDelay), icon.name)
        } else {
            return display === QQC2.ToolButton.IconOnly ? height : implicitWidth
        }
        icon.name: showCancel ? "dialog-cancel" : "list-add"
        text: showCancel ?
            cancelText(SpectacleCore.captureTimeRemaining / 1000)
            : i18n("New Screenshot")
        onClicked: if (showCancel) {
            SpectacleCore.cancelScreenshot()
        } else {
            SpectacleCore.takeNewScreenshot()
        }
    }

    ToolButton {
        visible: root.showOptionsMenu
        icon.name: "configure"
        text: i18n("Options")
        down: pressed || contextWindow.optionsMenu.visible
        Accessible.role: Accessible.ButtonMenu
        // for some reason, y has to be set to get the correct y pos, but x shouldn't be
        onPressed: contextWindow.optionsMenu.popup(this)
    }
    ToolButton {
        visible: !root.showOptionsMenu
        icon.name: "configure"
        text: i18n("Configure...")
        onClicked: contextWindow.optionsMenu.showPreferencesDialog();
    }

    ToolButton {
        id: helpButton
        icon.name: "help-contents"
        text: i18n("Help")
        down: pressed || contextWindow.helpMenu.visible
        Accessible.role: Accessible.ButtonMenu
        // for some reason, y has to be set to get the correct y pos, but x shouldn't be
        onPressed: contextWindow.helpMenu.popup(this)
    }
}
