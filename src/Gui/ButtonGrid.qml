/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

Grid {
    id: root
    property int displayMode: QQC.AbstractButton.TextBesideIcon
    property int focusPolicy: Qt.StrongFocus
    readonly property bool mirrored: effectiveLayoutDirection === Qt.RightToLeft
    property bool animationsEnabled: false

    clip: childrenRect.width > width || childrenRect.height > height
    horizontalItemAlignment: Grid.AlignHCenter
    verticalItemAlignment: Grid.AlignVCenter
    spacing: Kirigami.Units.mediumSpacing
    // Rows and columns are set to defaults like this:
    // if (rows <= 0 && columns <= 0) { columns = 4; rows = (numVisible+3)/4; }
    // else if (rows <= 0) { rows = (numVisible+(columns-1))/columns; }
    // else if (columns <= 0) { columns = (numVisible+(rows-1))/rows; }
    columns: flow === Grid.LeftToRight ? -1 : 1
    rows: flow === Grid.TopToBottom ? -1 : 1
    move: Transition {
        enabled: root.animationsEnabled
        NumberAnimation { properties: "x,y"; duration: Kirigami.Units.longDuration; easing.type: Easing.OutCubic }
    }
    add: Transition {
        enabled: root.animationsEnabled
        NumberAnimation { properties: "x,y"; duration: Kirigami.Units.longDuration; easing.type: Easing.OutCubic }
    }
}
