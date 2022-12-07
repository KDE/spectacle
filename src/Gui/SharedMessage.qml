/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQml 2.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami

InlineMessage {
    id: root
    type: Kirigami.MessageType.Information
    text: !messageArgument ? i18n("Image shared")
        : i18n("The shared image link (<a href=\"%1\">%1</a>) has been copied to the clipboard.",
               messageArgument)
    // Not using showCloseButton because it toggles visible on this item,
    // making it harder to use with loaders.
    actions: Kirigami.Action {
        displayComponent: QQC2.ToolButton {
            icon.name: "dialog-close"
            onClicked: root.loader.state = "inactive"
        }
    }
    Timer {
        running: messageArgument ? true : false
        interval: 10000
        onTriggered: root.loader.state = "inactive"
    }
}
