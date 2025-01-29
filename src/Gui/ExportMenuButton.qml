/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

TtToolButton {
    // FIXME: make export menu actually work with videos
    visible: !SpectacleCore.videoMode
    icon.name: "document-share"
    text: i18nc("@action", "Export")
    down: pressed || (ExportMenu.visible && !FullMenu.visible)
    Accessible.role: Accessible.ButtonMenu
    onPressed: ExportMenu.popup(this)
}
