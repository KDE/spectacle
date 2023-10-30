/* SPDX-FileCopyrightText: 2023 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

ColumnLayout {
    ColumnLayout {
        visible: !VideoPlatform.isRecording
        spacing: Kirigami.Units.mediumSpacing

        Repeater {
            model: SpectacleCore.recordingModeModel
            delegate: QQC.Button {
                id: button
                Layout.fillWidth: true
                leftPadding: Kirigami.Units.mediumSpacing + QmlUtils.fontMetrics.descent
                rightPadding: Kirigami.Units.mediumSpacing + QmlUtils.fontMetrics.descent
                topPadding: Kirigami.Units.mediumSpacing
                bottomPadding: Kirigami.Units.mediumSpacing
                text: model.display
                onClicked: SpectacleCore.recordingModeModel.startRecording(model.index, Settings.includePointer)
            }
        }
        Kirigami.Heading {
            Layout.fillWidth: true
            topPadding: -recordingSettingsMetrics.descent + parent.spacing
            bottomPadding: -recordingSettingsMetrics.descent + parent.spacing
            text: i18n("Recording Settings")
            level: 3
            FontMetrics {
                id: recordingSettingsMetrics
            }
        }
        QQC.CheckBox {
            Layout.fillWidth: true
            text: i18n("Include mouse pointer")
            QQC.ToolTip.text: i18n("Show the mouse cursor in the screen recording.")
            QQC.ToolTip.delay: Kirigami.Units.toolTipDelay
            QQC.ToolTip.visible: hovered
            checked: Settings.includePointer
            onToggled: Settings.includePointer = checked
        }
    }
    ColumnLayout {
        visible: VideoPlatform.isRecording
        QQC.Button {
            Layout.fillWidth: true
            text: i18n("Finish recording")
            onClicked: VideoPlatform.finishRecording()
        }
    }
    Item {
        Layout.fillHeight: true
    }
}
