/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private
import org.kde.kquickimageeditor

Grid {
    id: root
    property int focusPolicy: Qt.StrongFocus
    property real buttonHeight: undoButton.implicitHeight
    property bool animationsEnabled: true
    spacing: Kirigami.Units.mediumSpacing
    columns: flow === Grid.LeftToRight ? visibleChildren.length : 1
    rows: flow === Grid.TopToBottom ? visibleChildren.length : 1

    add: Transition {
        enabled: root.animationsEnabled
        NumberAnimation { properties: "x,y"; duration: Kirigami.Units.longDuration; easing.type: Easing.OutCubic }
    }

    TtToolButton {
        id: undoButton
        enabled: SpectacleCore.annotationDocument.undoStackDepth > 0
        height: root.buttonHeight
        focusPolicy: root.focusPolicy
        display: QQC.ToolButton.IconOnly
        text: i18n("Undo")
        icon.name: "edit-undo"
        autoRepeat: true
        onClicked: SpectacleCore.annotationDocument.undo()
    }

    TtToolButton {
        enabled: SpectacleCore.annotationDocument.redoStackDepth > 0
        height: root.buttonHeight
        focusPolicy: root.focusPolicy
        display: QQC.ToolButton.IconOnly
        text: i18n("Redo")
        icon.name: "edit-redo"
        autoRepeat: true
        onClicked: SpectacleCore.annotationDocument.redo()
    }

    QQC.ToolSeparator {
        height: root.flow === Grid.TopToBottom ? implicitWidth : parent.height
        width: root.flow === Grid.TopToBottom ? parent.width : implicitWidth
    }
}
