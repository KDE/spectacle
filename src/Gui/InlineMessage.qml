/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import org.kde.kirigami as Kirigami

Kirigami.InlineMessage {
    visible: true
    readonly property AnimatedLoader loader: parent
    property var messageArgument: ""
    property bool video: false
    icon.name: switch (type) {
    case Kirigami.MessageType.Error: return "dialog-error"
    case Kirigami.MessageType.Warning: return "dialog-warning"
    case Kirigami.MessageType.Positive: return "dialog-ok-apply"
    case Kirigami.MessageType.Information: return "dialog-information"
    default: return ""
    }
    onLinkActivated: Qt.openUrlExternally(link)
}
