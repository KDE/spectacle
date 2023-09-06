/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami
import org.kde.spectacle.private 1.0

ColumnLayout {
    spacing: Kirigami.Units.mediumSpacing
    Repeater {
        model: SpectacleCore.captureModeModel
        delegate: QQC2.DelayButton {
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
            QQC2.ToolTip.text: model.shortcuts
            QQC2.ToolTip.visible: (hovered || pressed) && model.shortcuts.length > 0
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            onClicked: if (showCancel) {
                SpectacleCore.cancelScreenshot()
            } else {
                Settings.captureMode = model.captureMode
                SpectacleCore.takeNewScreenshot()
            }
        }
    }
}
