/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

TtToolButton {
    icon.name: "application-menu"
    text: i18nc("@action", "Menu")
    display: TtToolButton.IconOnly
    down: pressed || FullMenu.visible
    Accessible.role: Accessible.ButtonMenu
    onPressed: FullMenu.popup(this)
}
