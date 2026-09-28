/* SPDX-FileCopyrightText: 2023 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

ColumnLayout {
    id: root
    spacing: Kirigami.Units.mediumSpacing

    // GIF and WebP can't carry an audio track, so the audio options are
    // disabled for those formats.
    readonly property bool audioSupported: SpectacleCore.videoPlatform.formatSupportsAudio(Settings.preferredVideoFormat)

    QQC.CheckBox {
        Layout.fillWidth: true
        text: i18nc("@option:check", "Include mouse pointer")
        QQC.ToolTip.text: i18nc("@info:tooltip", "Show the mouse cursor in the screen recording.")
        QQC.ToolTip.delay: Kirigami.Units.toolTipDelay
        QQC.ToolTip.visible: hovered
        checked: Settings.videoIncludePointer
        onToggled: Settings.videoIncludePointer = checked
    }
    RowLayout {
        spacing: root.spacing
        QQC.Label {
            enabled: root.audioSupported
            text: i18nc("@label:listbox", "Audio:")
        }
        QQC.Button {
            Layout.fillWidth: true
            enabled: root.audioSupported
            icon.name: AudioDeviceModel.iconName
            // Device names like "B&O" would otherwise get a mnemonic
            text: AudioDeviceModel.summary.replace(/&/g, "&&")
            down: pressed || AudioDeviceMenu.visible
            Accessible.role: Accessible.ButtonMenu
            // Explain why this is disabled, since a disabled control doesn't get
            // hover events and can't show its tooltip.
            Accessible.description: root.audioSupported ? "" : audioUnsupportedMessage.text
            QQC.ToolTip.text: i18nc("@info:tooltip", "Choose the audio devices to include in the recording. Nothing selected means the recording has no sound.")
            QQC.ToolTip.delay: Kirigami.Units.toolTipDelay
            QQC.ToolTip.visible: hovered && !AudioDeviceMenu.visible
            onPressed: AudioDeviceMenu.popup(this)
        }
    }
    Kirigami.InlineMessage {
        id: audioUnsupportedMessage
        Layout.fillWidth: true
        type: Kirigami.MessageType.Information
        text: i18nc("@info", "The selected video format does not support audio.")
        visible: !root.audioSupported
    }
}
