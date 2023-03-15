/* SPDX-FileCopyrightText: 2023 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQml 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami
import org.kde.spectacle.private 1.0

ColumnLayout {
    ColumnLayout {
        visible: !SpectacleCore.isRecording
        spacing: Kirigami.Units.mediumSpacing
        Kirigami.Heading {
            Layout.fillWidth: true
            topPadding: -captureHeadingMetrics.descent
            bottomPadding: -captureHeadingMetrics.descent + parent.spacing
            text: i18n("New screen recording")
            level: 3
            FontMetrics {
                id: captureHeadingMetrics
            }
        }
        Repeater {
            model: SpectacleCore.recordingModeModel
            delegate: QQC2.Button {
                id: button
                Layout.fillWidth: true
                leftPadding: Kirigami.Units.mediumSpacing + fontMetrics.descent
                rightPadding: Kirigami.Units.mediumSpacing + fontMetrics.descent
                topPadding: Kirigami.Units.mediumSpacing
                bottomPadding: Kirigami.Units.mediumSpacing
                text: model.display
                onClicked: SpectacleCore.recordingModeModel.startRecording(model.index, Settings.includePointer)
            }
        }
        Kirigami.Heading {
            Layout.fillWidth: true
            topPadding: -captureHeadingMetrics.descent + parent.spacing
            bottomPadding: -captureHeadingMetrics.descent + parent.spacing
            text: i18n("Recording Settings")
            level: 3
        }
        QQC2.CheckBox {
            Layout.fillWidth: true
            text: i18n("Include mouse pointer")
            QQC2.ToolTip.text: i18n("Show the mouse cursor in the screen recording.")
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            QQC2.ToolTip.visible: hovered
            checked: Settings.includePointer
            onToggled: Settings.includePointer = checked
        }
        RowLayout {
            visible: SpectacleCore.supportedVideoFormats.length > 1
            Layout.fillWidth: true
            QQC2.Label {
                text: i18nc("@label:listbox", "Video format:")
            }
            QQC2.ComboBox {
                id: formatCombo
                Layout.fillWidth: true
                model: SpectacleCore.supportedVideoFormats
                onActivated: {
                    const fmt = valueAt(index);
                    SpectacleCore.videoFormat = fmt;
                }
                Binding {
                    target: formatCombo
                    property: "currentIndex"
                    value: Math.max(0,  SpectacleCore.supportedVideoFormats.indexOf(SpectacleCore.videoFormat));
                }
            }
        }
    }
    ColumnLayout {
        visible: SpectacleCore.isRecording
        QQC2.Button {
            Layout.fillWidth: true
            text: i18n("Finish recording")
            onClicked: SpectacleCore.finishRecording()
        }
    }
    Item {
        Layout.fillHeight: true
    }
}
