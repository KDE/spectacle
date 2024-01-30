/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC
import org.kde.spectacle.private
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
            buttonHeight: QmlUtils.iconTextButtonHeight
            height: flow === Grid.LeftToRight ? buttonHeight : implicitHeight
            width: flow === Grid.TopToBottom ? buttonHeight : implicitWidth
            focusPolicy: root.focusPolicy
            flow: root.flow
            spacing: root.spacing
        }
    }

    QQC.ButtonGroup {
        id: toolGroup
        exclusive: true
        onClicked: Settings.annotationToolType = AnnotationDocument.tool.type
    }

    component ToolButton: QQC.ToolButton {
        id: button
        implicitHeight: QmlUtils.iconTextButtonHeight
        focusPolicy: root.focusPolicy
        display: root.displayMode
        checkable: true
        QQC.ToolTip {
            id: tooltip
            text: button.text
            visible: (button.hovered || button.pressed)
                && button.display === QQC.ToolButton.IconOnly
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
        QQC.ButtonGroup.group: toolGroup
        text: i18n("None")
        icon.name: "transform-browse"
        checked: AnnotationDocument.tool.type === AnnotationTool.NoTool
        onClicked: AnnotationDocument.tool.type = AnnotationTool.NoTool
    }
    ToolButton {
        QQC.ButtonGroup.group: toolGroup
        text: i18n("Select")
        icon.name: "edit-select"
        enabled: AnnotationDocument.undoStackDepth > 0
        checked: AnnotationDocument.tool.type === AnnotationTool.SelectTool
        onClicked: AnnotationDocument.tool.type = AnnotationTool.SelectTool
    }
    ToolButton {
        QQC.ButtonGroup.group: toolGroup
        text: i18n("Freehand")
        icon.name: "draw-freehand"
        checked: AnnotationDocument.tool.type === AnnotationTool.FreeHandTool
        onClicked: AnnotationDocument.tool.type = AnnotationTool.FreeHandTool
    }
    ToolButton {
        QQC.ButtonGroup.group: toolGroup
        text: i18n("Highlighter")
        icon.name: "draw-highlight"
        checked: AnnotationDocument.tool.type === AnnotationTool.HighlightTool
        onClicked: AnnotationDocument.tool.type = AnnotationTool.HighlightTool
    }
    ToolButton {
        QQC.ButtonGroup.group: toolGroup
        text: i18n("Line")
        icon.name: "draw-line"
        checked: AnnotationDocument.tool.type === AnnotationTool.LineTool
        onClicked: AnnotationDocument.tool.type = AnnotationTool.LineTool
    }
    ToolButton {
        QQC.ButtonGroup.group: toolGroup
        text: i18n("Arrow")
        icon.name: "draw-arrow"
        checked: AnnotationDocument.tool.type === AnnotationTool.ArrowTool
        onClicked: AnnotationDocument.tool.type = AnnotationTool.ArrowTool
    }
    ToolButton {
        QQC.ButtonGroup.group: toolGroup
        text: i18n("Rectangle")
        icon.name: "draw-rectangle"
        checked: AnnotationDocument.tool.type === AnnotationTool.RectangleTool
        onClicked: AnnotationDocument.tool.type = AnnotationTool.RectangleTool
    }
    ToolButton {
        QQC.ButtonGroup.group: toolGroup
        text: i18n("Ellipse")
        icon.name: "draw-ellipse"
        checked: AnnotationDocument.tool.type === AnnotationTool.EllipseTool
        onClicked: AnnotationDocument.tool.type = AnnotationTool.EllipseTool
    }
    ToolButton {
        QQC.ButtonGroup.group: toolGroup
        text: i18n("Pixelate")
        icon.name: "pixelart-trace"
        checked: AnnotationDocument.tool.type === AnnotationTool.PixelateTool
        onClicked: AnnotationDocument.tool.type = AnnotationTool.PixelateTool
    }
    ToolButton {
        QQC.ButtonGroup.group: toolGroup
        text: i18n("Blur")
        icon.name: "blurfx"
        checked: AnnotationDocument.tool.type === AnnotationTool.BlurTool
        onClicked: AnnotationDocument.tool.type = AnnotationTool.BlurTool
    }
    ToolButton {
        QQC.ButtonGroup.group: toolGroup
        text: i18n("Text")
        icon.name: "draw-text"
        checked: AnnotationDocument.tool.type === AnnotationTool.TextTool
        onClicked: AnnotationDocument.tool.type = AnnotationTool.TextTool
    }
    ToolButton {
        QQC.ButtonGroup.group: toolGroup
        text: i18n("Number")
        icon.name: "draw-number"
        checked: AnnotationDocument.tool.type === AnnotationTool.NumberTool
        onClicked: AnnotationDocument.tool.type = AnnotationTool.NumberTool
    }

    Component.onCompleted: if (rememberToolType) {
        AnnotationDocument.tool.type = Settings.annotationToolType
    }
}
