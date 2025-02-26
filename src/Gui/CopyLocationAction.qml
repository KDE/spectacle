/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick.Templates as T

T.Action {
    icon.name: "edit-copy-path"
    text: i18nc("@action", "Copy Location")
    onTriggered: contextWindow.copyLocation()
}
