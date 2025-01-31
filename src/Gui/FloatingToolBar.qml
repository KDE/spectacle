/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

T.Pane {
    id: root
    property real radius: Kirigami.Units.mediumSpacing / 2 + background.border.width
    property real topLeftRadius: radius
    property real topRightRadius: radius
    property real bottomLeftRadius: radius
    property real bottomRightRadius: radius
    property real backgroundColorOpacity: 0.85

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    padding: Kirigami.Units.mediumSpacing
    spacing: Kirigami.Units.mediumSpacing

    background: FloatingBackground {
        color: Qt.rgba(root.palette.window.r,
                       root.palette.window.g,
                       root.palette.window.b, root.backgroundColorOpacity)
        border.color: Qt.rgba(root.palette.windowText.r,
                              root.palette.windowText.g,
                              root.palette.windowText.b, 0.2)
        radius: root.radius
        corners.topLeftRadius: root.topLeftRadius
        corners.topRightRadius: root.topRightRadius
        corners.bottomLeftRadius: root.bottomLeftRadius
        corners.bottomRightRadius: root.bottomRightRadius
        border.width: contextWindow.dprRound(1)
    }
}
