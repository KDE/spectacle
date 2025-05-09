/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

ListView {
    id: root
    model: InlineMessageModel
    interactive: false
    visible: height > 0
    height: count > 0 ? implicitHeight : 0
    implicitHeight: contentHeight
    contentWidth: width
    Behavior on height {
        NumberAnimation {
            duration: Kirigami.Units.shortDuration
            easing.type: Easing.OutCubic
        }
    }
    add: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            duration: Kirigami.Units.shortDuration
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            property: "y"
            duration: Kirigami.Units.shortDuration
            easing.type: Easing.OutCubic
        }
    }
    populate: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            duration: Kirigami.Units.shortDuration
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            property: "y"
            duration: Kirigami.Units.shortDuration
            easing.type: Easing.OutCubic
        }
    }
    remove: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1
            to: 0
            duration: Kirigami.Units.shortDuration
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            property: "y"
            duration: Kirigami.Units.shortDuration
            easing.type: Easing.OutCubic
        }
    }
    displaced: Transition {
        NumberAnimation {
            property: "y"
            duration: Kirigami.Units.shortDuration
            easing.type: Easing.OutCubic
        }
    }
    delegate: Kirigami.InlineMessage {
        id: delegate
        required property int index
        required property var model
        type: {
            const type = model.type
            if (type === InlineMessageModel.Error) {
                return Kirigami.MessageType.Error
            } else if (type === InlineMessageModel.Warning) {
                return Kirigami.MessageType.Warning
            }
            return Kirigami.MessageType.Information
        }
        text: model.text
        visible: true
        position: Kirigami.InlineMessage.Position.Header
        width: parent.width
        actions: {
            const type = model.type
            const data = model.data
            let actions = []
            if (data !== undefined) {
                actions.push(copyActionComponent.createObject(this, {"data": data}))
            }
            try {
                let url = new URL(data)
                if (url.protocol === "file:") {
                    actions.push(openFolderActionComponent.createObject(this, {"data": url}))
                }
            } catch (error) { }
            // Not using showCloseButton because it toggles visible on this item,
            // making it harder to use with loaders.
            actions.push(closeActionComponent.createObject(this, {"index": index}))
            return actions
        }
        onLinkActivated: (link) => Qt.openUrlExternally(link)
        Timer {
            running: switch (delegate.model.type) {
            case InlineMessageModel.Copied:
            case InlineMessageModel.Shared:
                return true
            default:
                return false
            }
            interval: 10000
            onTriggered: InlineMessageModel.pop(delegate.index)
        }
    }

    Component {
        id: copyActionComponent
        Kirigami.Action {
            required property var data
            displayHint: Kirigami.DisplayHint.IconOnly
            icon.name: "edit-copy"
            text: i18nc("@action:button", "Copy to Clipboard")
            onTriggered: contextWindow.copyToClipboard(data)
        }
    }
    Component {
        id: openFolderActionComponent
        Kirigami.Action {
            required property url data
            icon.name: "document-open-folder"
            text: i18nc("@action:button", "Open Containing Folder")
            onTriggered: contextWindow.openContainingFolder(data)
        }
    }
    Component {
        id: closeActionComponent
        Kirigami.Action {
            required property int index
            displayHint: Kirigami.DisplayHint.IconOnly
            icon.name: "dialog-close"
            text: i18nc("@action:button close inline notification", "Close")
            onTriggered: InlineMessageModel.pop(index)
        }
    }
}
