/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
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
        model: SpectacleCore.captureModeModel
        delegate: QQC.DelayButton {
            id: button
            readonly property bool showCancel: Settings.captureMode === model.captureMode && SpectacleCore.captureTimeRemaining > 0
            Layout.fillWidth: true
            leftPadding: Kirigami.Units.mediumSpacing + QmlUtils.fontMetrics.descent
            rightPadding: Kirigami.Units.mediumSpacing + QmlUtils.fontMetrics.descent
            topPadding: Kirigami.Units.mediumSpacing
            bottomPadding: Kirigami.Units.mediumSpacing
            // Delay doesn't really matter since we set
            // progress directly and have no transition
            delay: 1
            transition: null
            progress: Settings.captureMode === model.captureMode ?
                SpectacleCore.captureProgress : 0
            icon.name: showCancel ? "dialog-cancel" : ""
            text: showCancel ?
                i18np("Cancel (%1 second)", "Cancel (%1 seconds)",
                        Math.ceil(SpectacleCore.captureTimeRemaining / 1000))
                : model.display
            QQC.ToolTip.text: model.shortcuts
            QQC.ToolTip.visible: (hovered || pressed) && model.shortcuts.length > 0
            QQC.ToolTip.delay: Kirigami.Units.toolTipDelay
            onClicked: if (showCancel) {
                SpectacleCore.cancelScreenshot()
            } else {
                Settings.captureMode = model.captureMode
                SpectacleCore.takeNewScreenshot()
            }
        }
    }
}
