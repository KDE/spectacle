/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC
import org.kde.spectacle.private

TtToolButton {
    icon.name: "camera-photo"
    text: i18nc("@action select new screenshot mode", "New Screenshot")
    down: pressed || ScreenshotModeMenu.visible
    Accessible.role: Accessible.ButtonMenu
    onPressed: ScreenshotModeMenu.popup(this)
}
