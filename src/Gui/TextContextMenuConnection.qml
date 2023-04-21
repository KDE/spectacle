/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15

Connections {
    function onPressed(event) {
        if (event.button === Qt.RightButton) {
            TextContextMenu.popup(target)
        }
    }
}
