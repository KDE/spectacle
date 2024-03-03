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

        RecordingModeButtonsColumn {
            Layout.fillWidth: true
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
        RecordingSettingsColumn {
            Layout.fillWidth: true
        }
    }
    ColumnLayout {
        visible: VideoPlatform.isRecording
        QQC.Button {
            Layout.fillWidth: true
            text: i18n("Finish recording")
            onClicked: SpectacleCore.finishRecording()
        }
    }
    Item {
        Layout.fillHeight: true
    }
}
