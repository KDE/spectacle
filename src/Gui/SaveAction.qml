/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick.Templates as T
import org.kde.spectacle.private

T.Action {
    // We don't use this in video mode because the video is already
    // automatically saved and you can't edit the video.
    enabled: !SpectacleCore.videoMode
    icon.name: "document-save"
    text: i18nc("@action", "Save")
    onTriggered: contextWindow.save()
}
