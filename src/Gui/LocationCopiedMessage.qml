/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC
import org.kde.kirigami as Kirigami

InlineMessage {
    id: root
    type: Kirigami.MessageType.Information
    text: video ?
        i18nc("@info", "The video location has been copied to the clipboard.")
        : i18nc("@info", "The screenshot location has been copied to the clipboard.")
    // Not using showCloseButton because it toggles visible on this item,
    // making it harder to use with loaders.
    actions: Kirigami.Action {
        displayComponent: QQC.ToolButton {
            icon.name: "dialog-close"
            onClicked: root.loader.state = "inactive"
        }
    }
    Timer {
        running: true
        interval: 10000
        onTriggered: root.loader.state = "inactive"
    }
}
