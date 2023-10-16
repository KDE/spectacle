/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import org.kde.spectacle.private

Menu {
    id: root
    Instantiator {
        id: instantiator
        model: CaptureModeModel
        delegate: Action {
            required property var model
            text: model.display
            onTriggered: (source) => {
                Settings.captureMode = model.captureMode
                SpectacleCore.takeNewScreenshot()
            }
        }

        onObjectAdded: (index, object) => root.insertAction(index, object)
        onObjectRemoved: (index, object) => root.removeAction(object)
    }
}
