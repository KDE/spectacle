/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import org.kde.spectacle.private

TtToolButton {
    icon.name: "configure"
    text: i18nc("@action", "Options")
    down: pressed || OptionsMenu.visible
    Accessible.role: Accessible.ButtonMenu
    onPressed: OptionsMenu.popup(this)
}
