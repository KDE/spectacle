/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick.Templates as T

T.Action {
    icon.name: "dialog-ok"
    text: i18nc("@action accept selection", "Accept")
    onTriggered: contextWindow.accept()
}
