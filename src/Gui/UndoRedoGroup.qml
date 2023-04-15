/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQml 2.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami
import org.kde.spectacle.private 1.0
import "Annotations"

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

    QQC2.ToolButton {
        id: undoButton
        enabled: AnnotationDocument.undoStackDepth > 0
        height: root.buttonHeight
        focusPolicy: root.focusPolicy
        display: QQC2.ToolButton.IconOnly
        text: i18n("Undo")
        icon.name: "edit-undo"
        autoRepeat: true
        QQC2.ToolTip.text: text
        QQC2.ToolTip.visible: hovered || pressed
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        onClicked: AnnotationDocument.undo()
    }

    QQC2.ToolButton {
        enabled: AnnotationDocument.redoStackDepth > 0
        height: root.buttonHeight
        focusPolicy: root.focusPolicy
        display: QQC2.ToolButton.IconOnly
        text: i18n("Redo")
        icon.name: "edit-redo"
        autoRepeat: true
        QQC2.ToolTip.text: text
        QQC2.ToolTip.visible: hovered || pressed
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        onClicked: AnnotationDocument.redo()
    }

    QQC2.ToolSeparator {
        height: root.flow === Grid.TopToBottom ? implicitWidth : parent.height
        width: root.flow === Grid.TopToBottom ? parent.width : implicitWidth
    }
}
