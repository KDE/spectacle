/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick.Templates as T
import org.kde.spectacle.private

T.Action {
    icon.name: "camera-video"
    text: i18nc("@action switch to recording mode", "Recording")
    checkable: true
}
