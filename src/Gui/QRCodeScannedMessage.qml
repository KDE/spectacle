/* SPDX-FileCopyrightText: 2024 Dinesh Manajipet <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQml 2.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami

InlineMessage {
    id: root

    function sanitise(text : string) : string {
        return text.replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;')
            .replace(/'/g, '&#x27;');
    }

    function linkify(text) {
        // Thanks to: https://stackoverflow.com/questions/1500260/detect-urls-in-text-with-javascript
        var urlRegex =/(\b(https?|ftp|file):\/\/[-A-Z0-9+&@#\/%?=~_|!:,.;]*[-A-Z0-9+&@#\/%=~_|])/ig;
        return text.replace(urlRegex, function(url) {
            return '<a href="' + url + '">' + url + '</a>';
        });
    }

    type: Kirigami.MessageType.Information
    text: typeof messageArgument === "string" ? i18n("QR Code found: %1", linkify(sanitise(messageArgument))) : i18n("Found QR code with binary content.")
    actions: [
        Kirigami.Action {
            displayComponent: QQC2.ToolButton {
                icon.name: "edit-copy"
                onClicked: contextWindow.copyToClipboard(messageArgument)
            }
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
