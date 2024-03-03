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
            onClicked: SpectacleCore.startRecording(model.recordingMode, Settings.videoIncludePointer)
        }
    }
}
