/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

ButtonGrid {
    id: root
    property size imageSize: Qt.size(0, 0)
    property bool showSizeLabel: false
    property bool showUndoRedo: false
    property bool showNewScreenshotButton: true
    property bool showOptionsMenu: true

    component ToolButton: QQC.ToolButton {
        implicitHeight: QmlUtils.iconTextButtonHeight
        width: display === QQC.ToolButton.IconOnly ? height : implicitWidth
        focusPolicy: root.focusPolicy
        display: root.displayMode
        QQC.ToolTip.text: text
        QQC.ToolTip.visible: (hovered || pressed) && display === QQC.ToolButton.IconOnly
        QQC.ToolTip.delay: Kirigami.Units.toolTipDelay
    }

    AnimatedLoader {
        id: sizeLabelLoader
        state: root.showSizeLabel && root.imageSize.width > 0 && root.imageSize.height > 0 ?
            "active" : "inactive"
        sourceComponent: SizeLabel {
            height: QmlUtils.iconTextButtonHeight
            size: root.imageSize
            leftPadding: Kirigami.Units.mediumSpacing + QmlUtils.fontMetrics.descent
            rightPadding: leftPadding
        }
    }

    AnimatedLoader {
        state: root.showUndoRedo ? "active" : "inactive"
        sourceComponent: UndoRedoGroup {
            animationsEnabled: root.animationsEnabled
            buttonHeight: QmlUtils.iconTextButtonHeight
            focusPolicy: root.focusPolicy
            flow: root.flow
            spacing: root.spacing
        }
    }

    // We don't show this in video mode because the video is already automatically saved.
    // and you can't edit the video.
    ToolButton {
        visible: !SpectacleCore.videoMode
        icon.name: "document-save"
        text: i18n("Save")
        onClicked: contextWindow.save()
    }

    ToolButton {
        icon.name: "document-save-as"
        text: i18n("Save As...")
        onClicked: contextWindow.saveAs()
    }

    // We don't show this in video mode because you can't copy raw video to the clipboard,
    // or at least not elegantly.
    ToolButton {
        visible: !SpectacleCore.videoMode
        icon.name: "edit-copy"
        text: i18n("Copy")
        onClicked: contextWindow.copyImage()
    }

    // We only show this in video mode to save space in screenshot mode
    ToolButton {
        visible: SpectacleCore.videoMode
        icon.name: "edit-copy-path"
        text: i18n("Copy Location")
        onClicked: contextWindow.copyLocation()
    }

    ToolButton {
        // FIXME: make export menu actually work with videos
        visible: !SpectacleCore.videoMode
        icon.name: "document-share"
        text: i18n("Export")
        down: pressed || ExportMenu.visible
        Accessible.role: Accessible.ButtonMenu
        onPressed: ExportMenu.popup(this)
    }

    ToolButton {
        id: annotationsButton
        icon.name: "edit-image"
        text: i18nc("@action:button edit screenshot", "Edit…")
        visible: !SpectacleCore.videoMode
        checkable: true
        checked: contextWindow.annotating
        onToggled: contextWindow.annotating = checked
    }

    ToolButton {
        // Can't rely on checked since clicking also toggles checked
        readonly property bool showCancel: SpectacleCore.captureTimeRemaining > 0
        readonly property real cancelWidth: QmlUtils.getButtonSize(display, cancelText(Settings.captureDelay), icon.name).width

        function cancelText(seconds) {
            return i18np("Cancel (%1 second)", "Cancel (%1 seconds)", Math.ceil(seconds))
        }

        visible: root.showNewScreenshotButton
        checked: showCancel
        width: if (showCancel) {
            return cancelWidth
        } else {
            return display === QQC.ToolButton.IconOnly ? height : implicitWidth
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
        down: pressed || OptionsMenu.visible
        Accessible.role: Accessible.ButtonMenu
        onPressed: OptionsMenu.popup(this)
    }
    ToolButton {
        visible: !root.showOptionsMenu
        icon.name: "configure"
        text: i18n("Configure...")
        onClicked: OptionsMenu.showPreferencesDialog();
    }

    ToolButton {
        id: helpButton
        icon.name: "help-contents"
        text: i18n("Help")
        down: pressed || HelpMenu.visible
        Accessible.role: Accessible.ButtonMenu
        onPressed: HelpMenu.popup(this)
    }
}
