/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

ListView {
    id: root
    model: InlineMessageModel
    interactive: false
    visible: height > 0
    height: implicitHeight
    implicitHeight: contentHeight
    contentWidth: width
    Behavior on height {
        NumberAnimation {
            duration: Kirigami.Units.longDuration
            easing.type: Easing.OutCubic
        }
    }
    add: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            duration: Kirigami.Units.longDuration
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            property: "y"
            duration: Kirigami.Units.longDuration
            easing.type: Easing.OutCubic
        }
    }
    populate: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            duration: Kirigami.Units.longDuration
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            property: "y"
            duration: Kirigami.Units.longDuration
            easing.type: Easing.OutCubic
        }
    }
    remove: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1
            to: 0
            duration: Kirigami.Units.longDuration
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            property: "y"
            duration: Kirigami.Units.longDuration
            easing.type: Easing.OutCubic
        }
    }
    displaced: Transition {
        NumberAnimation {
            property: "y"
            duration: Kirigami.Units.longDuration
            easing.type: Easing.OutCubic
        }
    }
    delegate: Kirigami.InlineMessage {
        id: delegate
        required property int index
        required property var model
        function makeActions() {
            let actions = []
            // Incubate actions to prevent error messages when the delegate is
            // quickly created and destroyed.
            function pushAction(incubator) {
                const actionIndex = actions.length
                if (incubator.status !== Component.Ready) {
                    actions.push(null)
                    incubator.onStatusChanged = (status) => {
                        if (status === Component.Ready) {
                            delegate.actions[actionIndex] = incubator.object
                        }
                    }
                } else {
                    actions.push(incubator.object)
                }
            }
            if (model.controls & InlineMessageModel.Control.OpenContainingFolder) {
                try {
                    let url = new URL(model.data)
                    if (url.protocol === "file:") {
                        pushAction(openFolderActionComponent.incubateObject(this, {"url": url}))
                    }
                } catch (error) { }
            }
            if (model.controls & InlineMessageModel.Control.Copy) {
                pushAction(copyActionComponent.incubateObject(this, {"data": model.data}))
            }
            if (model.controls & InlineMessageModel.Control.Busy) {
                pushAction(busyActionComponent.incubateObject(this, {}))
            }
            // Not using showCloseButton because it toggles visible on this item.
            // This makes it not work as well with animations from the list view.
            pushAction(closeActionComponent.incubateObject(this, {"index": index}))
            return actions
        }
        implicitHeight: implicitContentHeight + topPadding + bottomPadding
        z: -index // stack newer items under older items so there's no ugly visual overlap
        type: model.type
        text: model.text
        visible: !timer.delay
        position: Kirigami.InlineMessage.Position.Header
        width: root.width
        actions: visible ? makeActions() : []
        onLinkActivated: (link) => Qt.openUrlExternally(link)
        Timer {
            id: timer
            property bool delay: delegate.model.behaviors & InlineMessageModel.Behavior.Delay
            property bool timeout: delegate.model.behaviors & InlineMessageModel.Behavior.Timeout
            running: delay || timeout
            interval: delay ? 200 : 10000
            onTriggered: {
                if (delay) {
                    delegate.visible = true
                    if (timeout) {
                        delay = false
                        timer.restart()
                    }
                } else if (timeout) {
                    InlineMessageModel.pop(delegate.index)
                }
            }
        }
    }

    Component {
        id: openFolderActionComponent
        Kirigami.Action {
            required property url url
            icon.name: "document-open-folder"
            text: i18nc("@action:button", "Open Containing Folder")
            onTriggered: InlineMessageModel.openContainingFolder(url)
        }
    }
    Component {
        id: copyActionComponent
        Kirigami.Action {
            required property var data
            displayHint: Kirigami.DisplayHint.IconOnly
            icon.name: "edit-copy"
            text: i18nc("@action:button", "Copy to Clipboard")
            onTriggered: InlineMessageModel.copyToClipboard(data)
        }
    }
    Component {
        id: closeActionComponent
        Kirigami.Action {
            id: kirigamiAction
            required property int index
            icon.name: "dialog-close"
            text: i18nc("@action:button close inline notification", "Close")
            onTriggered: InlineMessageModel.pop(index)
            displayComponent: QQC.ToolButton {
                display: QQC.AbstractButton.IconOnly
                action: kirigamiAction
            }
        }
    }
    Component {
        id: busyActionComponent
        Kirigami.Action {
            displayComponent: QQC.BusyIndicator {
                running: true
            }
        }
    }
}
