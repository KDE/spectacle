/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import org.kde.spectacle.private

TtToolButton {
    icon.name: "help-contents"
    text: i18nc("@action", "Help")
    down: pressed || HelpMenu.visible
    Accessible.role: Accessible.ButtonMenu
    onPressed: HelpMenu.popup(this)
}
