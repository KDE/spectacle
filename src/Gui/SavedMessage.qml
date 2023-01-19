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
    property bool video: false
    text: (video ? i18n("The video was saved as <a href=\"%1\">%2</a>",
                messageArgument,
                contextWindow.baseFileName(messageArgument)) : i18n("The screenshot was saved as <a href=\"%1\">%2</a>",
                messageArgument,
                contextWindow.baseFileName(messageArgument)))
    actions: [
        QQC2.Action {
            icon.name: "document-open-folder"
            text: i18n("Open Containing Folder")
            onTriggered: contextWindow.openContainingFolder(messageArgument)
        },
        // Not using showCloseButton because it toggles visible on this item,
        // making it harder to use with loaders.
        Kirigami.Action {
            displayComponent: QQC2.ToolButton {
                icon.name: "dialog-close"
                onClicked: root.loader.state = "inactive"
            }
        }
    ]
}
