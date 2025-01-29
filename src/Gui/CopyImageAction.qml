/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick.Templates as T
import org.kde.spectacle.private

T.Action {
    // We don't use this in video mode because you can't copy raw video to the
    // clipboard, or at least not elegantly.
    enabled: !SpectacleCore.videoMode
    icon.name: "edit-copy"
    text: i18nc("@action", "Copy")
    onTriggered: contextWindow.copyImage()
}
