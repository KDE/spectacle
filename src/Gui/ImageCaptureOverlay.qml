/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls as QQC
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

import "Annotations"

/**
 * This page is shown when using the rectangular region capture mode.
 *
 * - There is a `contextWindow` context property that can be used to
 * access the instance of the CaptureWindow.
 * - There is a `SelectionEditor` singleton instance that can be used to
 * access the instance of SelectionEditor instantiated by contextWindow.
 * - There is a `Selection` singleton instance that can be used to
 * access the size and position of the selected area set by SelectionEditor.
 */
MouseArea {
    // This needs to be a mousearea in orcer for the proper mouse events to be correctly filtered
    id: root
    focus: true
    acceptedButtons: Qt.LeftButton | Qt.RightButton
    hoverEnabled: true
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    anchors.fill: parent

    AnnotationEditor {
        id: annotations
        anchors.fill: parent
        visible: true
        enabled: contextWindow.annotating
        viewportRect: Geometry.mapFromPlatformRect(screenToFollow.geometry, screenToFollow.devicePixelRatio)
    }

    component Overlay: Rectangle {
        color: Settings.useLightMaskColor ? "white" : "black"
        opacity: 0.5
        LayoutMirroring.enabled: false
    }
    Overlay { // top / full overlay when nothing selected
        id: topOverlay
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: selectionRectangle.visible ? selectionRectangle.top : parent.bottom
        opacity: if (SelectionEditor.selection.empty
            && (!annotations.document.tool.isNoTool || annotations.document.undoStackDepth > 0)) {
            return 0
        } else if (SelectionEditor.selection.empty) {
            return 0.25
        } else {
            return 0.5
        }
        Behavior on opacity {
            NumberAnimation {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.OutCubic
            }
        }
    }
    Overlay { // bottom
        id: bottomOverlay
        anchors.left: parent.left
        anchors.top: selectionRectangle.visible ? selectionRectangle.bottom : undefined
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: selectionRectangle.visible && height > 0
    }
    Overlay { // left
        anchors {
            left: topOverlay.left
            top: topOverlay.bottom
            right: selectionRectangle.visible ? selectionRectangle.left : undefined
            bottom: bottomOverlay.top
        }
        visible: selectionRectangle.visible && height > 0 && width > 0
    }
    Overlay { // right
        anchors {
            left: selectionRectangle.visible ? selectionRectangle.right : undefined
            top: topOverlay.bottom
            right: topOverlay.right
            bottom: bottomOverlay.top
        }
        visible: selectionRectangle.visible && height > 0 && width > 0
    }

    Rectangle {
        id: selectionRectangle
        enabled: annotations.document.tool.isNoTool
        color: "transparent"
        border.color: if (enabled) {
            return palette.active.highlight
        } else if (Settings.useLightMaskColor) {
            return "black"
        } else {
            return "white"
        }
        border.width: contextWindow.dprRound(1)
        visible: !SelectionEditor.selection.empty && Geometry.rectIntersects(Qt.rect(x,y,width,height), Qt.rect(0,0,parent.width, parent.height))
        x: SelectionEditor.selection.x - border.width - annotations.viewportRect.x
        y: SelectionEditor.selection.y - border.width - annotations.viewportRect.y
        width: SelectionEditor.selection.width + border.width * 2
        height: SelectionEditor.selection.height + border.width * 2

        LayoutMirroring.enabled: false
        LayoutMirroring.childrenInherit: true
    }

    Item {
        x: -annotations.viewportRect.x
        y: -annotations.viewportRect.y
        enabled: selectionRectangle.enabled
        component SelectionHandle: Handle {
            visible: enabled && selectionRectangle.visible
                && SelectionEditor.dragLocation === SelectionEditor.None
                && Geometry.rectIntersects(Qt.rect(x,y,width,height), annotations.viewportRect)
            fillColor: selectionRectangle.border.color
        }

        SelectionHandle {
            edges: Qt.TopEdge | Qt.LeftEdge
            x: SelectionEditor.handlesRect.x
            y: SelectionEditor.handlesRect.y
        }
        SelectionHandle {
            edges: Qt.LeftEdge
            x: SelectionEditor.handlesRect.x
            y: SelectionEditor.handlesRect.y + SelectionEditor.handlesRect.height/2 - height/2
        }
        SelectionHandle {
            edges: Qt.LeftEdge | Qt.BottomEdge
            x: SelectionEditor.handlesRect.x
            y: SelectionEditor.handlesRect.y + SelectionEditor.handlesRect.height - height
        }
        SelectionHandle {
            edges: Qt.TopEdge
            x: SelectionEditor.handlesRect.x + SelectionEditor.handlesRect.width/2 - width/2
            y: SelectionEditor.handlesRect.y
        }
        SelectionHandle {
            edges: Qt.BottomEdge
            x: SelectionEditor.handlesRect.x + SelectionEditor.handlesRect.width/2 - width/2
            y: SelectionEditor.handlesRect.y + SelectionEditor.handlesRect.height - height
        }
        SelectionHandle {
            edges: Qt.RightEdge
            x: SelectionEditor.handlesRect.x + SelectionEditor.handlesRect.width - width
            y: SelectionEditor.handlesRect.y + SelectionEditor.handlesRect.height/2 - height/2
        }
        SelectionHandle {
            edges: Qt.TopEdge | Qt.RightEdge
            x: SelectionEditor.handlesRect.x + SelectionEditor.handlesRect.width - width
            y: SelectionEditor.handlesRect.y
        }
        SelectionHandle {
            edges: Qt.RightEdge | Qt.BottomEdge
            x: SelectionEditor.handlesRect.x + SelectionEditor.handlesRect.width - width
            y: SelectionEditor.handlesRect.y + SelectionEditor.handlesRect.height - height
        }
    }

    ShortcutsTextBox {
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
        }
        visible: opacity > 0 && Settings.showCaptureInstructions
        // Assume SelectionEditor covers all screens.
        // Use parent's coordinate system.
        opacity: root.containsMouse
            && !contains(mapFromItem(root, root.mouseX, root.mouseY))
            && !root.pressed
            && annotations.document.tool.isNoTool
            && !mtbDragHandler.active
            && !atbDragHandler.active
            && !Geometry.rectIntersects(SelectionEditor.handlesRect, Qt.rect(x, y, width, height))
        Behavior on opacity {
            NumberAnimation {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.OutCubic
            }
        }
    }

    Item { // separate item because it needs to be above the stuff defined above
        width: SelectionEditor.screensRect.width
        height: SelectionEditor.screensRect.height
        x: -annotations.viewportRect.x
        y: -annotations.viewportRect.y

        // Magnifier
        Loader {
            id: magnifierLoader
            readonly property point targetPoint: {
                if (SelectionEditor.magnifierLocation === SelectionEditor.FollowMouse) {
                    return SelectionEditor.mousePosition
                } else {
                    let x = -width
                    let y = -height
                    if (SelectionEditor.magnifierLocation === SelectionEditor.TopLeft
                        || SelectionEditor.magnifierLocation === SelectionEditor.Left
                        || SelectionEditor.magnifierLocation === SelectionEditor.BottomLeft) {
                        x = SelectionEditor.selection.left
                    } else if (SelectionEditor.magnifierLocation === SelectionEditor.TopRight
                        || SelectionEditor.magnifierLocation === SelectionEditor.Right
                        || SelectionEditor.magnifierLocation === SelectionEditor.BottomRight) {
                        x = SelectionEditor.selection.right
                    } else if (SelectionEditor.magnifierLocation === SelectionEditor.Top
                        || SelectionEditor.magnifierLocation === SelectionEditor.Bottom) {
                        if (SelectionEditor.dragLocation !== SelectionEditor.None) {
                            x = SelectionEditor.mousePosition.x
                        } else {
                            x = SelectionEditor.selection.horizontalCenter
                        }
                    }
                    if (SelectionEditor.magnifierLocation === SelectionEditor.TopLeft
                        || SelectionEditor.magnifierLocation === SelectionEditor.Top
                        || SelectionEditor.magnifierLocation === SelectionEditor.TopRight) {
                        y = SelectionEditor.selection.top
                    } else if (SelectionEditor.magnifierLocation === SelectionEditor.BottomLeft
                        || SelectionEditor.magnifierLocation === SelectionEditor.Bottom
                        || SelectionEditor.magnifierLocation === SelectionEditor.BottomRight) {
                        y = SelectionEditor.selection.bottom
                    } else if (SelectionEditor.magnifierLocation === SelectionEditor.Left
                        || SelectionEditor.magnifierLocation === SelectionEditor.Right) {
                        if (SelectionEditor.dragLocation !== SelectionEditor.None) {
                            y = SelectionEditor.mousePosition.y
                        } else {
                            y = SelectionEditor.selection.verticalCenter
                        }
                    }
                    return Qt.point(x, y)
                }
            }
            readonly property rect rect: {
                let margin = Kirigami.Units.gridUnit
                let x = targetPoint.x + margin
                let y = targetPoint.y + margin
                if (SelectionEditor.magnifierLocation !== SelectionEditor.FollowMouse) {
                    if (SelectionEditor.magnifierLocation === SelectionEditor.TopLeft
                        || SelectionEditor.magnifierLocation === SelectionEditor.Left
                        || SelectionEditor.magnifierLocation === SelectionEditor.BottomLeft) {
                        x = targetPoint.x - width - margin
                    } else if (SelectionEditor.magnifierLocation === SelectionEditor.TopRight
                        || SelectionEditor.magnifierLocation === SelectionEditor.Right
                        || SelectionEditor.magnifierLocation === SelectionEditor.BottomRight) {
                        x = targetPoint.x + margin
                    } else if (SelectionEditor.magnifierLocation === SelectionEditor.Top
                        || SelectionEditor.magnifierLocation === SelectionEditor.Bottom) {
                        x = targetPoint.x - width / 2
                    }

                    if (SelectionEditor.magnifierLocation === SelectionEditor.TopLeft
                        || SelectionEditor.magnifierLocation === SelectionEditor.Top
                        || SelectionEditor.magnifierLocation === SelectionEditor.TopRight) {
                        y = targetPoint.y - width - margin
                    } else if (SelectionEditor.magnifierLocation === SelectionEditor.BottomLeft
                        || SelectionEditor.magnifierLocation === SelectionEditor.Bottom
                        || SelectionEditor.magnifierLocation === SelectionEditor.BottomRight) {
                        y = targetPoint.y + margin
                    } else if (SelectionEditor.magnifierLocation === SelectionEditor.Left
                        || SelectionEditor.magnifierLocation === SelectionEditor.Right) {
                        y = targetPoint.y - height / 2
                    }
                }
                return Geometry.rectBounded(dprRound(x), dprRound(y), width, height, SelectionEditor.screensRect)
            }
            x: rect.x
            y: rect.y
            z: 100
            visible: SelectionEditor.showMagnifier
                && SelectionEditor.magnifierLocation !== SelectionEditor.None
                && Geometry.rectIntersects(rect, annotations.viewportRect)
            active: Settings.showMagnifier !== Settings.ShowMagnifierNever
            sourceComponent: Magnifier {
                viewport: annotations
                targetPoint: magnifierLoader.targetPoint
            }
        }

        Connections {
            target: SelectionEditor.selection
            function onEmptyChanged() {
                if (!SelectionEditor.selection.empty
                    && (mainToolBar.rememberPosition || atbLoader.rememberPosition)) {
                    mainToolBar.rememberPosition = false
                    atbLoader.rememberPosition = false
                }
            }
        }

        // Main ToolBar
        FloatingToolBar {
            id: mainToolBar
            property bool rememberPosition: false
            readonly property int valignment: {
                if (SelectionEditor.selection.empty) {
                    return 0
                }
                if (3 * height + topPadding + Kirigami.Units.mediumSpacing
                    <= SelectionEditor.screensRect.height - SelectionEditor.handlesRect.bottom
                ) {
                    return Qt.AlignBottom
                } else if (3 * height + bottomPadding + Kirigami.Units.mediumSpacing
                    <= SelectionEditor.handlesRect.top
                ) {
                    return Qt.AlignTop
                } else {
                    // At the bottom of the inside of the selection rect.
                    return Qt.AlignBaseline
                }
            }
            readonly property bool normallyVisible: {
                let emptyHovered = (root.containsMouse || annotations.hovered) && SelectionEditor.selection.empty
                let menuVisible = ExportMenu.visible
                menuVisible |= OptionsMenu.visible
                menuVisible |= HelpMenu.visible
                let pressed = SelectionEditor.dragLocation || annotations.anyPressed
                return (emptyHovered || !SelectionEditor.selection.empty || menuVisible) && !pressed
            }
            Binding on x {
                value: {
                    const v = SelectionEditor.selection.empty ? (root.width - mainToolBar.width) / 2 + annotations.viewportRect.x
                                                   : SelectionEditor.selection.horizontalCenter - mainToolBar.width / 2
                    return Math.max(mainToolBar.leftPadding, // min value
                           Math.min(contextWindow.dprRound(v),
                                    SelectionEditor.screensRect.width - mainToolBar.width - mainToolBar.rightPadding)) // max value
                }
                when: mainToolBar.normallyVisible && !mainToolBar.rememberPosition
                restoreMode: Binding.RestoreNone
            }
            Binding on y {
                value: {
                    let v = 0
                    // put above selection if not enough room below selection
                    if (mainToolBar.valignment & Qt.AlignTop) {
                        v = SelectionEditor.handlesRect.top
                            - mainToolBar.height - mainToolBar.bottomPadding
                    } else if (mainToolBar.valignment & Qt.AlignBottom) {
                        v = SelectionEditor.handlesRect.bottom + mainToolBar.topPadding
                    } else if (mainToolBar.valignment & Qt.AlignBaseline) {
                        v = Math.min(SelectionEditor.selection.bottom, SelectionEditor.handlesRect.bottom - Kirigami.Units.gridUnit)
                            - mainToolBar.height - mainToolBar.bottomPadding
                    } else {
                        v = (mainToolBar.height / 2) - mainToolBar.parent.y
                    }
                    return contextWindow.dprRound(v)
                }
                when: mainToolBar.normallyVisible && !mainToolBar.rememberPosition
                restoreMode: Binding.RestoreNone
            }
            visible: opacity > 0
            opacity: normallyVisible && Geometry.rectIntersects(Qt.rect(x,y,width,height), annotations.viewportRect)
            Behavior on opacity {
                NumberAnimation {
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.OutCubic
                }
            }
            layer.enabled: true // improves the visuals of the opacity animation
            focusPolicy: Qt.NoFocus
            contentItem: ButtonGrid {
                id: mainToolBarContents
                SizeLabel {
                    height: QmlUtils.iconTextButtonHeight
                    size: {
                        const sz = SelectionEditor.selection.empty
                            ? Qt.size(SelectionEditor.screensRect.width,
                                        SelectionEditor.screensRect.height)
                            : SelectionEditor.selection.size
                        return Geometry.rawSize(sz, SelectionEditor.devicePixelRatio)
                    }
                    leftPadding: Kirigami.Units.mediumSpacing + QmlUtils.fontMetrics.descent
                    rightPadding: leftPadding
                }
                TtToolButton {
                    focusPolicy: Qt.NoFocus
                    visible: action.enabled
                    action: SaveAction {}
                }
                TtToolButton {
                    focusPolicy: Qt.NoFocus
                    action: SaveAsAction {}
                }
                TtToolButton {
                    focusPolicy: Qt.NoFocus
                    visible: action.enabled
                    action: CopyImageAction {}
                }
                ExportMenuButton {
                    focusPolicy: Qt.NoFocus
                }
                NewScreenshotToolButton {
                    focusPolicy: Qt.NoFocus
                }
                FullMenuButton {
                    focusPolicy: Qt.NoFocus
                }
            }

            DragHandler { // parent is contentItem and parent is a read-only property
                id: mtbDragHandler
                enabled: SelectionEditor.selection.empty
                target: mainToolBar
                acceptedButtons: Qt.LeftButton
                margin: mainToolBar.padding
                xAxis.minimum: annotations.viewportRect.x
                xAxis.maximum: annotations.viewportRect.x + root.width - mainToolBar.width
                yAxis.minimum: annotations.viewportRect.y
                yAxis.maximum: annotations.viewportRect.y + root.height - mainToolBar.height
                cursorShape: enabled ?
                    (active ? Qt.ClosedHandCursor : Qt.OpenHandCursor)
                    : undefined
                onActiveChanged: if (active && !mainToolBar.rememberPosition) {
                    mainToolBar.rememberPosition = true
                }
            }
        }

        AnimatedLoader {
            id: atbLoader
            property bool rememberPosition: false
            readonly property int valignment: mainToolBar.valignment & (Qt.AlignTop | Qt.AlignBaseline) ?
                Qt.AlignTop : Qt.AlignBottom
            active: visible && mainToolBar.visible
            onActiveChanged: if (!active && rememberPosition
                && !contextWindow.annotating) {
                rememberPosition = false
            }
            state: mainToolBar.normallyVisible
                && contextWindow.annotating ? "active" : "inactive"

            Binding on x {
                value: {
                    const min = mainToolBar.x
                    const target = contextWindow.dprRound(mainToolBar.x + (mainToolBar.width - atbLoader.width) / 2)
                    const max = mainToolBar.x + mainToolBar.width - atbLoader.width
                    return Math.max(min, Math.min(target, max))
                }
                when: !atbLoader.rememberPosition
                restoreMode: Binding.RestoreNone
            }
            Binding on y {
                value: contextWindow.dprRound(atbLoader.valignment & Qt.AlignTop ?
                    mainToolBar.y - atbLoader.height - Kirigami.Units.mediumSpacing
                    : mainToolBar.y + mainToolBar.height + Kirigami.Units.mediumSpacing)
                when: !atbLoader.rememberPosition
                restoreMode: Binding.RestoreNone
            }

            DragHandler { // parented to contentItem
                id: atbDragHandler
                enabled: SelectionEditor.selection.empty
                acceptedButtons: Qt.LeftButton
                xAxis.minimum: annotations.viewportRect.x
                xAxis.maximum: annotations.viewportRect.x + root.width - atbLoader.width
                yAxis.minimum: annotations.viewportRect.y
                yAxis.maximum: annotations.viewportRect.y + root.height - atbLoader.height
                cursorShape: enabled ?
                    (active ? Qt.ClosedHandCursor : Qt.OpenHandCursor)
                    : undefined
                onActiveChanged: if (active && !atbLoader.rememberPosition) {
                    atbLoader.rememberPosition = true
                }
            }

            sourceComponent: FloatingToolBar {
                id: annotationsToolBar
                focusPolicy: Qt.NoFocus
                contentItem: AnnotationsToolBarContents {
                    id: annotationsContents
                    displayMode: QQC.AbstractButton.IconOnly
                    focusPolicy: Qt.NoFocus
                }

                topLeftRadius: optionsToolBar.visible
                    && optionsToolBar.x === 0
                    && atbLoader.valignment & Qt.AlignTop ? 0 : radius
                topRightRadius: optionsToolBar.visible
                    && optionsToolBar.x === width - optionsToolBar.width
                    && atbLoader.valignment & Qt.AlignTop ? 0 : radius
                bottomLeftRadius: optionsToolBar.visible
                    && optionsToolBar.x === 0
                    && atbLoader.valignment & Qt.AlignBottom ? 0 : radius
                bottomRightRadius: optionsToolBar.visible
                    && optionsToolBar.x === width - optionsToolBar.width
                    && atbLoader.valignment & Qt.AlignBottom ? 0 : radius

                // Exists purely for cosmetic reasons to make the border of
                // optionsToolBar that meets annotationsToolBar look better
                Rectangle {
                    id: borderBg
                    z: -1
                    visible: optionsToolBar.visible
                    opacity: optionsToolBar.opacity
                    parent: annotationsToolBar
                    x: optionsToolBar.x + annotationsToolBar.background.border.width
                    y: atbLoader.valignment & Qt.AlignTop ?
                        optionsToolBar.y + optionsToolBar.height : optionsToolBar.y
                    width: optionsToolBar.width - annotationsToolBar.background.border.width * 2
                    height: contextWindow.dprRound(1)
                    color: annotationsToolBar.background.color
                }

                AnimatedLoader {
                    id: optionsToolBar
                    parent: annotationsToolBar
                    x: {
                        let targetX = annotationsContents.x
                        const checkedButton = annotationsContents.checkedButton
                        if (checkedButton) {
                            targetX += checkedButton.x + (checkedButton.width - width) / 2
                        }
                        return Math.max(0, // min value
                               Math.min(contextWindow.dprRound(targetX),
                                        parent.width - width)) // max value
                    }
                    y: atbLoader.valignment & Qt.AlignTop ?
                        -optionsToolBar.height + borderBg.height
                        : optionsToolBar.height - borderBg.height
                    state: if (SpectacleCore.annotationDocument.tool.options !== AnnotationTool.NoOptions
                              || (SpectacleCore.annotationDocument.tool.type === AnnotationTool.SelectTool
                                 && SpectacleCore.annotationDocument.selectedItem.options !== AnnotationTool.NoOptions)
                    ) {
                        return "active"
                    } else {
                        return "inactive"
                    }
                    sourceComponent: FloatingToolBar {
                        focusPolicy: Qt.NoFocus
                        contentItem: AnnotationOptionsToolBarContents {
                            displayMode: QQC.AbstractButton.IconOnly
                            focusPolicy: Qt.NoFocus
                        }
                        topLeftRadius: atbLoader.valignment & Qt.AlignBottom && x >= 0 ? 0 : radius
                        topRightRadius: atbLoader.valignment & Qt.AlignBottom && x + width <= annotationsToolBar.width ? 0 : radius
                        bottomLeftRadius: atbLoader.valignment & Qt.AlignTop && x >= 0 ? 0 : radius
                        bottomRightRadius: atbLoader.valignment & Qt.AlignTop && x + width <= annotationsToolBar.width ? 0 : radius
                    }
                }
            }
        }
    }

    // FIXME: This shortcut only exists here because spectacle interprets "Ctrl+Shift+,"
    // as "Ctrl+Shift+<" for some reason unless we use a QML Shortcut.
    Shortcut {
        sequences: [StandardKey.Preferences]
        onActivated: contextWindow.showPreferencesDialog()
    }

    Connections {
        target: contextWindow
        function onVisibilityChanged(visibility) {
            if (visibility !== Window.Hidden && visibility !== Window.Minimized) {
                contextWindow.raise()
                if (root.containsMouse) {
                    contextWindow.requestActivate()
                }
            }
        }
    }
}
