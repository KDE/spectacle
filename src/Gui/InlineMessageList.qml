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
    visible: count > 0
    height: visible ? implicitHeight : 0
    implicitHeight: contentItem.childrenRect.height
    contentWidth: width
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
    delegate: Loader {
        id: delegate
        required property int index
        required property string qmlFile
        required property var properties
        width: parent.width
        Connections {
            target: delegate.item
            function onClosed() {
                InlineMessageModel.removeRow(delegate.index)
            }
        }
        Component.onCompleted: delegate.setSource(delegate.qmlFile, delegate.properties)
    }
    Behavior on height {
        NumberAnimation {
            duration: Kirigami.Units.shortDuration
            easing.type: Easing.OutCubic
        }
    }
}
