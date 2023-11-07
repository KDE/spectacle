/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC
import org.kde.kirigami as Kirigami

InlineMessage {
    id: root
    type: Kirigami.MessageType.Information
    text: (video ? i18n("The video was saved as <a href=\"%1\">%2</a>",
                messageArgument,
                contextWindow.baseFileName(messageArgument)) : i18n("The screenshot was saved as <a href=\"%1\">%2</a>",
                messageArgument,
                contextWindow.baseFileName(messageArgument)))
    actions: [
        QQC.Action {
            icon.name: "document-open-folder"
            text: i18n("Open Containing Folder")
            onTriggered: contextWindow.openContainingFolder(messageArgument)
        },
        // Not using showCloseButton because it toggles visible on this item,
        // making it harder to use with loaders.
        Kirigami.Action {
            displayComponent: QQC.ToolButton {
                icon.name: "dialog-close"
                onClicked: root.loader.state = "inactive"
            }
        }
    ]
}
