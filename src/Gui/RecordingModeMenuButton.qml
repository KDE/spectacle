/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC
import org.kde.spectacle.private

TtToolButton {
    icon.name: "camera-video"
    text: i18nc("@action select recording mode", "Recording")
    down: pressed || RecordingModeMenu.visible
    Accessible.role: Accessible.ButtonMenu
    onPressed: RecordingModeMenu.popup(this)
}
