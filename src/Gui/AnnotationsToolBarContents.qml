/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls as QQC
import org.kde.spectacle.private
import org.kde.kquickimageeditor

ButtonGrid {
    id: root
    property bool showUndoRedo: true
    property bool showNoneButton: false
    property alias checkedButton: toolGroup.checkedButton
    readonly property alias usingCropTool: cropToolButton.checked

    readonly property AnnotationTool tool: SpectacleCore.annotationDocument.tool
    readonly property int toolType: tool?.type ?? AnnotationTool.NoTool

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
    }

    QQC.ToolTip {
        id: tooltip
        property T.AbstractButton target: null
        function shouldShow(target: T.AbstractButton) : bool {
            return target && (target.hovered || target.pressed)
                && target.display === QQC.ToolButton.IconOnly
        }
        parent: target || root
        text: {
            let name = target?.text ?? ""
            let extraHelp = target?.extraHelp ?? ""
            if (!extraHelp) {
                return name
            }
            extraHelp = extraHelp.replace(/\n/g, "<br>")
            // NOTE: We can't use CSS here because it's pretty broken in Qt Quick.
            // You either get the CSS as plain text in your output or rich text
            // formatting is just completely stopped.
            return `<html>${name}<br><font size="-1">${extraHelp}</font></html>`
        }
        visible: shouldShow(target)
        Binding on x {
            value: root.mirrored ?
                -root.parent.leftPadding - tooltip.width
                : (tooltip.target ? tooltip.target.width + root.parent.rightPadding : 0)
            when: root.flow === Grid.TopToBottom && tooltip.target
            restoreMode: Binding.RestoreBindingOrValue
        }
        Binding on y {
            value: tooltip.target ? // target check needed to prevent warnings
                dprRound((tooltip.target.height - tooltip.height) / 2) : 0
            when: root.flow === Grid.TopToBottom && tooltip.target
            restoreMode: Binding.RestoreBindingOrValue
        }
    }

    component ToolButton: QQC.ToolButton {
        id: button
        property string extraHelp: ""
        implicitHeight: QmlUtils.iconTextButtonHeight
        focusPolicy: root.focusPolicy
        display: root.displayMode
        checkable: true
        containmentMask: Item {
            parent: button
            readonly property rect rect: root.flow === Grid.TopToBottom ?
                Qt.rect(-root.x, -root.spacing / 2, parent.width + root.x * 2, parent.height + root.spacing)
                : Qt.rect(-root.spacing / 2, -root.y, parent.width + root.spacing, parent.height + root.y * 2)
            x: rect.x
            y: rect.y
            width: rect.width
            height: rect.height
        }
        Binding {
            target: tooltip
            property: "target"
            value: button
            when: tooltip.shouldShow(button)
            restoreMode: Binding.RestoreNone
        }
    }

    // Used in the image viewer where one might want to drag and drop or pan around a viewport
    Loader {
        active: root.showNoneButton
        visible: active
        sourceComponent: ToolButton {
            QQC.ButtonGroup.group: toolGroup
            text: i18nc("@action:intoolbar no tool, to allow dragging", "None")
            icon.name: "transform-browse"
            onClicked: root.tool.type = AnnotationTool.NoTool
            // Setting checked in onCompleted so clicking the crop tool doesn't check this instead.
            Component.onCompleted: {
                checked = root.toolType === AnnotationTool.NoTool
            }
        }
    }
    ToolButton {
        id: cropToolButton
        QQC.ButtonGroup.group: toolGroup
        text: i18nc("@action:intoolbar crop image tool", "Crop")
        icon.name: "transform-crop"
        onClicked: root.tool.type = AnnotationTool.NoTool
        // Setting checked in onCompleted so that 2 clicks aren't needed to check this.
        Component.onCompleted: {
            checked = root.toolType === AnnotationTool.NoTool && !root.showNoneButton
        }
    }
    ToolButton {
        QQC.ButtonGroup.group: toolGroup
        text: i18nc("@action:intoolbar select annotation tool", "Select")
        icon.name: "edit-select"
        enabled: SpectacleCore.annotationDocument.undoStackDepth > 0
        checked: root.toolType === AnnotationTool.SelectTool
        onClicked: root.tool.type = AnnotationTool.SelectTool
    }
    ToolButton {
        id: freehandButton
        QQC.ButtonGroup.group: toolGroup
        text: i18nc("@action:intoolbar freehand line tool", "Freehand")
        extraHelp: i18nc("@info:tooltip", "Shift: Snap to straight lines")
        icon.name: "draw-freehand"
        checked: root.toolType === AnnotationTool.FreehandTool
        onClicked: root.tool.type = AnnotationTool.FreehandTool
    }
    ToolButton {
        QQC.ButtonGroup.group: toolGroup
        text: i18nc("@action:intoolbar highlighter line tool", "Highlighter")
        extraHelp: freehandButton.extraHelp
        icon.name: "draw-highlight"
        checked: root.toolType === AnnotationTool.HighlighterTool
        onClicked: root.tool.type = AnnotationTool.HighlighterTool
    }
    ToolButton {
        id: lineButton
        QQC.ButtonGroup.group: toolGroup
        text: i18nc("@action:intoolbar straight line tool", "Line")
        extraHelp: i18nc("@info:tooltip", "Shift: Snap to 45 degree increments")
        icon.name: "draw-line"
        checked: root.toolType === AnnotationTool.LineTool
        onClicked: root.tool.type = AnnotationTool.LineTool
    }
    ToolButton {
        QQC.ButtonGroup.group: toolGroup
        text: i18nc("@action:intoolbar arrow line tool", "Arrow")
        extraHelp: lineButton.extraHelp
        icon.name: "draw-arrow"
        checked: root.toolType === AnnotationTool.ArrowTool
        onClicked: root.tool.type = AnnotationTool.ArrowTool
    }
    ToolButton {
        id: rectangleButton
        QQC.ButtonGroup.group: toolGroup
        text: i18nc("@action:intoolbar rectangle tool", "Rectangle")
        extraHelp: i18nc("@info:tooltip",
            "Shift: Snap to diagonal movements\nControl: Resize from center")
        icon.name: "draw-rectangle"
        checked: root.toolType === AnnotationTool.RectangleTool
        onClicked: root.tool.type = AnnotationTool.RectangleTool
    }
    ToolButton {
        QQC.ButtonGroup.group: toolGroup
        text: i18nc("@action:intoolbar ellipse tool", "Ellipse")
        extraHelp: rectangleButton.extraHelp
        icon.name: "draw-ellipse"
        checked: root.toolType === AnnotationTool.EllipseTool
        onClicked: root.tool.type = AnnotationTool.EllipseTool
    }
    ToolButton {
        QQC.ButtonGroup.group: toolGroup
        text: i18nc("@action:intoolbar pixelate tool", "Pixelate")
        extraHelp: rectangleButton.extraHelp
        icon.name: "pixelate"
        checked: root.toolType === AnnotationTool.PixelateTool
        onClicked: root.tool.type = AnnotationTool.PixelateTool
    }
    ToolButton {
        QQC.ButtonGroup.group: toolGroup
        text: i18nc("@action:intoolbar blur tool", "Blur")
        extraHelp: rectangleButton.extraHelp
        icon.name: "blur"
        checked: root.toolType === AnnotationTool.BlurTool
        onClicked: root.tool.type = AnnotationTool.BlurTool
    }
    ToolButton {
        QQC.ButtonGroup.group: toolGroup
        text: i18nc("@action:intoolbar text tool", "Text")
        icon.name: "draw-text"
        checked: root.toolType === AnnotationTool.TextTool
        onClicked: root.tool.type = AnnotationTool.TextTool
    }
    ToolButton {
        QQC.ButtonGroup.group: toolGroup
        text: i18nc("@action:intoolbar number stamp tool", "Number")
        icon.name: "draw-number"
        checked: root.toolType === AnnotationTool.NumberTool
        onClicked: root.tool.type = AnnotationTool.NumberTool
    }
}
