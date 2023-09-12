/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import org.kde.kirigami as Kirigami

Loader {
    id: loader
    property int animationDuration: Kirigami.Units.shortDuration
    active: visible
    visible: opacity > 0
    // Using states because they stay in sync better than Behavior animations
    state: "active"
    states: [
        State {
            name: "active"
            PropertyChanges {
                target: loader
                opacity: 1
            }
        },
        State {
            name: "inactive"
            PropertyChanges {
                target: loader
                opacity: 0
            }
        }
    ]
    transitions: Transition {
        NumberAnimation {
            property: "opacity"
            duration: loader.animationDuration
            easing.type: Easing.OutCubic
        }
    }
}
