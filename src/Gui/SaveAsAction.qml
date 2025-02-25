/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick.Templates as T

T.Action {
    icon.name: "document-save-as"
    text: i18nc("@action", "Save As…")
    onTriggered: contextWindow.saveAs()
}
