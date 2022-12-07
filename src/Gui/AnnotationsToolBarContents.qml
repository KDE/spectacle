/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQml 2.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami
import org.kde.spectacle.private 1.0
import "Annotations"

ButtonGrid {
    id: root
    property bool showUndoRedo: true
    property bool rememberToolType: false
    property alias checkedButton: toolGroup.checkedButton

    Loader {
        visible: active
        active: root.showUndoRedo
        sourceComponent: UndoRedoGroup {
            animationsEnabled: root.animationsEnabled
            buttonHeight: root.fullButtonHeight
            height: flow === Grid.LeftToRight ? buttonHeight : implicitHeight
            width: flow === Grid.TopToBottom ? buttonHeight : implicitWidth
            focusPolicy: root.focusPolicy
            flow: root.flow
            spacing: root.spacing
        }
    }

    QQC2.ButtonGroup {
        id: toolGroup
        exclusive: true
        onClicked: Settings.annotationToolType = AnnotationDocument.tool.type
    }

    component ToolButton: QQC2.ToolButton {
        id: button
        height: root.fullButtonHeight
        focusPolicy: root.focusPolicy
        display: root.displayMode
        QQC2.ToolTip {
            id: tooltip
            text: button.text
            visible: (button.hovered || button.pressed)
                && button.display === QQC2.ToolButton.IconOnly
            Binding on x {
                value: root.mirrored ?
                    -root.parent.leftPadding - tooltip.width
                    : button.width + root.parent.rightPadding
                when: root.flow === Grid.TopToBottom
                restoreMode: Binding.RestoreBindingOrValue
            }
            Binding on y {
                value: contextWindow.dprRound((button.height - tooltip.height) / 2)
                when: root.flow === Grid.TopToBottom
                restoreMode: Binding.RestoreBindingOrValue
            }
        }
    }

    ToolButton {
        QQC2.ButtonGroup.group: toolGroup
        text: i18n("None")
        icon.name: "transform-browse"
        checked: AnnotationDocument.tool.type === AnnotationDocument.None
        onClicked: AnnotationDocument.tool.type = AnnotationDocument.None
    }
    ToolButton {
        QQC2.ButtonGroup.group: toolGroup
        text: i18n("Select")
        icon.name: "edit-select"
        enabled: AnnotationDocument.undoStackDepth > 0
        checked: AnnotationDocument.tool.type === AnnotationDocument.ChangeAction
        onClicked: AnnotationDocument.tool.type = AnnotationDocument.ChangeAction
    }
    ToolButton {
        QQC2.ButtonGroup.group: toolGroup
        text: i18n("Freehand")
        icon.name: "draw-freehand"
        checked: AnnotationDocument.tool.type === AnnotationDocument.FreeHand
        onClicked: AnnotationDocument.tool.type = AnnotationDocument.FreeHand
    }
    ToolButton {
        QQC2.ButtonGroup.group: toolGroup
        text: i18n("Highlighter")
        icon.name: "draw-highlight"
        checked: AnnotationDocument.tool.type === AnnotationDocument.Highlight
        onClicked: AnnotationDocument.tool.type = AnnotationDocument.Highlight
    }
    ToolButton {
        QQC2.ButtonGroup.group: toolGroup
        text: i18n("Line")
        icon.name: "draw-line"
        checked: AnnotationDocument.tool.type === AnnotationDocument.Line
        onClicked: AnnotationDocument.tool.type = AnnotationDocument.Line
    }
    ToolButton {
        QQC2.ButtonGroup.group: toolGroup
        text: i18n("Arrow")
        icon.name: "draw-arrow"
        checked: AnnotationDocument.tool.type === AnnotationDocument.Arrow
        onClicked: AnnotationDocument.tool.type = AnnotationDocument.Arrow
    }
    ToolButton {
        QQC2.ButtonGroup.group: toolGroup
        text: i18n("Rectangle")
        icon.name: "draw-rectangle"
        checked: AnnotationDocument.tool.type === AnnotationDocument.Rectangle
        onClicked: AnnotationDocument.tool.type = AnnotationDocument.Rectangle
    }
    ToolButton {
        QQC2.ButtonGroup.group: toolGroup
        text: i18n("Ellipse")
        icon.name: "draw-ellipse"
        checked: AnnotationDocument.tool.type === AnnotationDocument.Ellipse
        onClicked: AnnotationDocument.tool.type = AnnotationDocument.Ellipse
    }
    ToolButton {
        QQC2.ButtonGroup.group: toolGroup
        text: i18n("Pixelate")
        icon.name: "pixelart-trace"
        checked: AnnotationDocument.tool.type === AnnotationDocument.Pixelate
        onClicked: AnnotationDocument.tool.type = AnnotationDocument.Pixelate
    }
    ToolButton {
        QQC2.ButtonGroup.group: toolGroup
        text: i18n("Blur")
        icon.name: "blurfx"
        checked: AnnotationDocument.tool.type === AnnotationDocument.Blur
        onClicked: AnnotationDocument.tool.type = AnnotationDocument.Blur
    }
    ToolButton {
        QQC2.ButtonGroup.group: toolGroup
        text: i18n("Text")
        icon.name: "draw-text"
        checked: AnnotationDocument.tool.type === AnnotationDocument.Text
        onClicked: AnnotationDocument.tool.type = AnnotationDocument.Text
    }
    ToolButton {
        QQC2.ButtonGroup.group: toolGroup
        text: i18n("Number")
        icon.name: "" //TODO: Needs proper icon
        checked: AnnotationDocument.tool.type === AnnotationDocument.Number
        onClicked: AnnotationDocument.tool.type = AnnotationDocument.Number
    }

    Component.onCompleted: if (rememberToolType) {
        AnnotationDocument.tool.type = Settings.annotationToolType
    }
}
