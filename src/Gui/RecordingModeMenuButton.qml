/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC
import org.kde.spectacle.private

TtToolButton {
    enabled: SpectacleCore.videoPlatform.supportedRecordingModes !== VideoPlatform.NoRecordingModes
    icon.name: "camera-video"
    text: i18nc("@action select new recording mode", "New Recording")
    down: pressed || RecordingModeMenu.visible
    Accessible.role: Accessible.ButtonMenu
    onPressed: RecordingModeMenu.popup(this)
    hoverEnabled: true
    QQC.ToolTip.visible: !enabled && hovered
    QQC.ToolTip.text: i18nc("@info:tooltip", "This feature is not supported on the current platform")
}
