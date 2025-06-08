/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Shapes
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
    readonly property rect viewportRect: Geometry.mapFromPlatformRect(screenToFollow.geometry, screenToFollow.devicePixelRatio)
    readonly property AnnotationDocument document: annotationsLoader.item?.document ?? null
    focus: true
    acceptedButtons: Qt.LeftButton | Qt.RightButton
    hoverEnabled: true
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    anchors.fill: parent
    enabled: !SpectacleCore.videoPlatform.isRecording

    AnimatedLoader {
        id: annotationsLoader
        anchors.fill: parent
        state: !SpectacleCore.videoMode ? "active" : "inactive"
        animationDuration: Kirigami.Units.veryLongDuration
        sourceComponent: AnnotationEditor {
            enabled: contextWindow.annotating
            viewportRect: root.viewportRect
        }
    }

    component ToolButton : TtToolButton {
        focusPolicy: Qt.NoFocus
    }

    component ToolBarSizeLabel: SizeLabel {
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

    component Overlay: Rectangle {
        color: Settings.useLightMaskColor ? "white" : "black"
        opacity: if (SpectacleCore.videoPlatform.isRecording
            || (annotationsLoader.visible
                && SelectionEditor.selection.empty
                && root.document && (!root.document.tool.isNoTool
                                     || root.document.undoStackDepth > 0))) {
            return 0
        } else if (SelectionEditor.selection.empty) {
            return 0.25
        } else {
            return 0.5
        }
        LayoutMirroring.enabled: false
        Behavior on opacity {
            NumberAnimation {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.OutCubic
            }
        }
    }
    Overlay { // top / full overlay when nothing selected
        id: topOverlay
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: selectionRectangle.visible ? selectionRectangle.top : parent.bottom
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

    AnimatedLoader {
        anchors.centerIn: parent
        visible: opacity > 0 && !SpectacleCore.videoPlatform.isRecording
        state: topOverlay.opacity === 0.25 ? "active" : "inactive"
        sourceComponent: Kirigami.Heading {
            id: cropToolHelp
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: i18nc("@info basic crop tool explanation", "Click and drag to make a selection.")
            padding: cropToolHelpMetrics.height - cropToolHelpMetrics.descent
            leftPadding: cropToolHelpMetrics.height
            rightPadding: cropToolHelpMetrics.height
            background: FloatingBackground {
                radius: cropToolHelpMetrics.height
                color: Qt.alpha(palette.window, 0.9)
            }
            FontMetrics {
                id: cropToolHelpMetrics
                font: cropToolHelp.font
            }
        }
    }


    DashedOutline {
        id: selectionRectangle
        // We need to be a bit careful about staying out of the recorded area
        function roundPos(value: real) : real {
            return SpectacleCore.videoPlatform.isRecording ? dprFloor(value - (strokeWidth + 1 / Screen.devicePixelRatio)) : dprRound(value - strokeWidth)
        }
        function roundSize(value: real) : real {
            return SpectacleCore.videoPlatform.isRecording ? dprCeil(value + (strokeWidth + 1 / Screen.devicePixelRatio)) : dprRound(value + strokeWidth)
        }
        pathHints: ShapePath.PathLinear
        dashSvgPath: SpectacleCore.videoPlatform.isRecording ? svgPath : ""
        visible: !SelectionEditor.selection.empty
            && Geometry.rectIntersects(Qt.rect(x,y,width,height), Qt.rect(0,0,parent.width, parent.height))
        enabled: root.document?.tool.isNoTool || SpectacleCore.videoMode
        strokeWidth: dprRound(1)
        strokeColor: if (enabled) {
            return palette.active.highlight
        } else if (Settings.useLightMaskColor) {
            return "black"
        } else {
            return "white"
        }
        dashColor: SpectacleCore.videoPlatform.isRecording ? palette.active.base : strokeColor
        // We need to be a bit careful about staying out of the recorded area
        x: roundPos(SelectionEditor.selection.x - root.viewportRect.x)
        y: roundPos(SelectionEditor.selection.y - root.viewportRect.y)
        width: roundSize(SelectionEditor.selection.right - root.viewportRect.x) - x
        height: roundSize(SelectionEditor.selection.bottom - root.viewportRect.y) - y
    }

    Item {
        x: -root.viewportRect.x
        y: -root.viewportRect.y
        enabled: selectionRectangle.enabled
        component SelectionHandle: Handle {
            visible: enabled && selectionRectangle.visible
                && SelectionEditor.dragLocation === SelectionEditor.None
                && Geometry.rectIntersects(Qt.rect(x,y,width,height), root.viewportRect)
            fillColor: selectionRectangle.strokeColor
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

    Item { // separate item because it needs to be above the stuff defined above
        id: screensRectItem
        readonly property bool allowToolbars: {
            let emptyHovered = (root.containsMouse || annotationsLoader.item?.hovered) && SelectionEditor.selection.empty
            let menuVisible = ExportMenu.visible
            menuVisible |= OptionsMenu.visible
            menuVisible |= HelpMenu.visible
            menuVisible |= ScreenshotModeMenu.visible
            menuVisible |= RecordingModeMenu.visible
            let pressed = SelectionEditor.dragLocation || annotationsLoader.item?.anyPressed
            return !SpectacleCore.videoPlatform.isRecording && !pressed
                && (emptyHovered || !SelectionEditor.selection.empty || menuVisible)
        }
        width: SelectionEditor.screensRect.width
        height: SelectionEditor.screensRect.height
        x: -root.viewportRect.x
        y: -root.viewportRect.y

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
            visible: active
                && SelectionEditor.showMagnifier
                && SelectionEditor.magnifierLocation !== SelectionEditor.None
                && Geometry.rectIntersects(rect, root.viewportRect)
            active: !SpectacleCore.videoMode
                && SelectionEditor.showMagnifier !== Settings.ShowMagnifierNever
                && annotationsLoader.item !== null
                && (root.document?.tool.isNoTool ?? false)
            sourceComponent: Magnifier {
                viewport: annotationsLoader.item
                targetPoint: magnifierLoader.targetPoint
            }
        }

        AnimatedLoader {
            z: 100
            state: SelectionEditor.dragLocation ? "active" : "inactive"
            sourceComponent: SizeLabel {
                height: {
                    let h = dprRound(implicitHeight)
                    return h + h % 2
                }
                width: dprCeil(implicitWidth)
                x: {
                    let x = SelectionEditor.mousePosition.x + Kirigami.Units.gridUnit
                    if (x + width > root.viewportRect.right) {
                        x -= Kirigami.Units.gridUnit * 2 + width
                    }
                    return dprRound(x)
                }
                y: {
                    let y = SelectionEditor.mousePosition.y - height / 2
                    if (y < root.viewportRect.top) {
                        y += height / 2
                    } else if (y + height > root.viewportRect.bottom) {
                        y -= height / 2
                    }
                    return dprRound(y)
                }
                size: {
                    const sz = SelectionEditor.selection.empty
                        ? Qt.size(SelectionEditor.screensRect.width,
                                    SelectionEditor.screensRect.height)
                        : SelectionEditor.selection.size
                    return Geometry.rawSize(sz, SelectionEditor.devicePixelRatio)
                }
                padding: Kirigami.Units.smallSpacing
                leftPadding: QmlUtils.fontMetrics.descent + padding
                rightPadding: leftPadding
                background: FloatingBackground {
                    color: Qt.alpha(palette.window, 0.9)
                }
            }
        }

        Connections {
            target: SelectionEditor.selection
            function onRectChanged() {
                if (!SelectionEditor.selection.empty) {
                    if (mainToolBar.rememberPosition) {
                        mainToolBar.z = 0
                        mainToolBar.rememberPosition = false
                    }
                    if (ftbLoader.item?.rememberPosition) {
                        ftbLoader.z = 0
                        ftbLoader.item.rememberPosition = false
                    }
                    if (atbLoader.item?.rememberPosition) {
                        atbLoader.z = 0
                        atbLoader.item.rememberPosition = false
                    }
                }
            }
        }

        // Main ToolBar
        FloatingToolBar {
            id: mainToolBar
            readonly property rect rect: Qt.rect(x, y, width, height)
            property bool rememberPosition: false
            property alias dragging: mtbDragHandler.active
            Binding on x {
                value: Math.max(root.viewportRect.x + mainToolBar.leftPadding, // min value
                        Math.min(dprRound(root.viewportRect.x + (root.width - mainToolBar.width) / 2),
                                root.viewportRect.x + root.width - mainToolBar.width - mainToolBar.rightPadding)) // max value
                when: screensRectItem.allowToolbars && !mainToolBar.rememberPosition
                restoreMode: Binding.RestoreNone
            }
            Binding on y {
                value: dprRound(root.viewportRect.y + root.height - mainToolBar.height - mainToolBar.bottomPadding)
                when: screensRectItem.allowToolbars && !mainToolBar.rememberPosition
                restoreMode: Binding.RestoreNone
            }
            visible: opacity > 0 && !SpectacleCore.videoPlatform.isRecording
            opacity: screensRectItem.allowToolbars
                && (mainToolBar.rememberPosition
                    || SelectionEditor.selection.empty
                    || (!Geometry.rectIntersects(mainToolBar.rect, ftbLoader.rect)
                        && !Geometry.rectIntersects(mainToolBar.rect, SelectionEditor.handlesRect)))
            Behavior on opacity {
                NumberAnimation {
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.OutCubic
                }
            }
            layer.enabled: true // improves the visuals of the opacity animation
            focusPolicy: Qt.NoFocus
            contentItem: ButtonGrid {
                spacing: parent.spacing
                Loader {
                    id: mtbImageVideoContentLoader
                    visible: SelectionEditor.selection.empty
                    active: visible
                    sourceComponent: SpectacleCore.videoMode ? videoToolBarComponent : imageMainToolBarComponent
                }
                QQC.ToolSeparator {
                    visible: mtbImageVideoContentLoader.visible
                    height: QmlUtils.iconTextButtonHeight
                }
                ScreenshotModeMenuButton {
                    focusPolicy: Qt.NoFocus
                }
                RecordingModeMenuButton {
                    focusPolicy: Qt.NoFocus
                }
                OptionsMenuButton {
                    focusPolicy: Qt.NoFocus
                }
            }

            HoverHandler {
                target: mainToolBar
                margin: mainToolBar.padding
                cursorShape: enabled ?
                    (mtbDragHandler.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor)
                    : undefined
            }
            DragHandler { // parent is contentItem and parent is a read-only property
                id: mtbDragHandler
                target: mainToolBar
                acceptedButtons: Qt.LeftButton
                margin: mainToolBar.padding
                xAxis.minimum: root.viewportRect.x
                xAxis.maximum: root.viewportRect.x + root.width - mainToolBar.width
                yAxis.minimum: root.viewportRect.y
                yAxis.maximum: root.viewportRect.y + root.height - mainToolBar.height
                onActiveChanged: if (active) {
                    mainToolBar.z = 2
                    atbLoader.z = atbLoader.z > ftbLoader.z ? 1 : 0
                    ftbLoader.z = atbLoader.z < ftbLoader.z ? 1 : 0
                    if (!mainToolBar.rememberPosition) {
                        mainToolBar.rememberPosition = true
                    }
                }
            }
        }

        Component {
            id: imageMainToolBarComponent
            ButtonGrid {
                spacing: parent.parent.spacing
                ToolBarSizeLabel {}
                ToolButton {
                    display: TtToolButton.TextBesideIcon
                    visible: action.enabled
                    action: AcceptAction {}
                }
                ToolButton {
                    display: TtToolButton.IconOnly
                    visible: action.enabled
                    action: SaveAction {}
                }
                ToolButton {
                    display: TtToolButton.IconOnly
                    visible: action.enabled
                    action: SaveAsAction {}
                }
                ToolButton {
                    display: TtToolButton.IconOnly
                    visible: action.enabled
                    action: CopyImageAction {}
                }
                ToolButton {
                    display: TtToolButton.IconOnly
                    visible: action.enabled && !SpectacleCore.videoMode && SpectacleCore.ocrAvailable
                    action: OcrAction {}
                }
                ExportMenuButton {
                    focusPolicy: Qt.NoFocus
                }
            }
        }
        Component {
            id: imageFinalizerToolBarComponent
            ButtonGrid {
                spacing: parent.parent.spacing
                ToolBarSizeLabel {}
                ToolButton {
                    visible: action.enabled
                    action: AcceptAction {}
                }
                ToolButton {
                    visible: action.enabled
                    action: SaveAction {}
                }
                ToolButton {
                    visible: action.enabled
                    action: SaveAsAction {}
                }
                ToolButton {
                    visible: action.enabled
                    action: CopyImageAction {}
                }
                ToolButton {
                    visible: action.enabled && !SpectacleCore.videoMode && SpectacleCore.ocrAvailable
                    action: OcrAction {}
                }
                ExportMenuButton {
                    focusPolicy: Qt.NoFocus
                }
            }
        }
        Component {
            id: videoToolBarComponent
            ButtonGrid {
                spacing: parent.parent.spacing
                ToolBarSizeLabel {}
                ToolButton {
                    display: TtToolButton.TextBesideIcon
                    visible: action.enabled
                    action: RecordAction {}
                }
            }
        }

        Loader {
            id: atbLoader
            readonly property rect rect: Qt.rect(x, y, width, height)
            visible: annotationsLoader.visible
            active: visible
            z: 2
            sourceComponent: FloatingToolBar {
                id: annotationsToolBar
                property bool rememberPosition: false
                readonly property int valignment: {
                    if (SelectionEditor.handlesRect.top >= height + annotationsToolBar.bottomPadding || SelectionEditor.selection.empty) {
                        // the top of the top side of the selection
                        // or the top of the screen
                        return Qt.AlignTop
                    } else {
                        // the bottom of the top side of the selection
                        return Qt.AlignBottom
                    }
                }
                property alias dragging: dragHandler.active
                visible: opacity > 0
                opacity: screensRectItem.allowToolbars && Geometry.rectIntersects(atbLoader.rect, root.viewportRect)
                Behavior on opacity {
                    NumberAnimation {
                        duration: Kirigami.Units.longDuration
                        easing.type: Easing.OutCubic
                    }
                }
                // Can't use layer.enabled to improve opacity animation because
                // that makes the options toolbar invisible
                focusPolicy: Qt.NoFocus
                contentItem: AnnotationsToolBarContents {
                    id: annotationsContents
                    spacing: parent.spacing
                    displayMode: QQC.AbstractButton.IconOnly
                    focusPolicy: Qt.NoFocus
                }

                topLeftRadius: otbLoader.visible
                    && otbLoader.x === 0
                    && (annotationsToolBar.valignment & Qt.AlignTop)
                    && !SelectionEditor.selection.empty ? 0 : radius
                topRightRadius: otbLoader.visible
                    && otbLoader.x === width - otbLoader.width
                    && (annotationsToolBar.valignment & Qt.AlignTop)
                    && !SelectionEditor.selection.empty? 0 : radius
                bottomLeftRadius: otbLoader.visible
                    && otbLoader.x === 0
                    && ((annotationsToolBar.valignment & Qt.AlignBottom)
                    || SelectionEditor.selection.empty) ? 0 : radius
                bottomRightRadius: otbLoader.visible
                    && otbLoader.x === width - otbLoader.width
                    && ((annotationsToolBar.valignment & Qt.AlignBottom)
                    || SelectionEditor.selection.empty) ? 0 : radius

                Binding {
                    property: "x"
                    target: atbLoader
                    value: {
                        const v = SelectionEditor.selection.empty ? (root.width - annotationsToolBar.width) / 2 + root.viewportRect.x
                                                    : SelectionEditor.selection.horizontalCenter - annotationsToolBar.width / 2
                        return Math.max(annotationsToolBar.leftPadding, // min value
                            Math.min(dprRound(v),
                                        SelectionEditor.screensRect.width - annotationsToolBar.width - annotationsToolBar.rightPadding)) // max value
                    }
                    when: screensRectItem.allowToolbars && !annotationsToolBar.rememberPosition
                    restoreMode: Binding.RestoreNone
                }
                Binding {
                    property: "y"
                    target: atbLoader
                    value: {
                        let v = 0
                        if (SelectionEditor.selection.empty) {
                            v = root.viewportRect.y + annotationsToolBar.topPadding
                        } else if (annotationsToolBar.valignment & Qt.AlignTop) {
                            v = SelectionEditor.handlesRect.top - annotationsToolBar.height - annotationsToolBar.bottomPadding
                        } else if (annotationsToolBar.valignment & Qt.AlignBottom) {
                            v = Math.max(SelectionEditor.selection.top, SelectionEditor.handlesRect.top + Kirigami.Units.gridUnit)
                                + annotationsToolBar.topPadding
                        } else {
                            v = (root.height - annotationsToolBar.height) / 2 - parent.y
                        }
                        return dprRound(v)
                    }
                    when: screensRectItem.allowToolbars && !annotationsToolBar.rememberPosition
                    restoreMode: Binding.RestoreNone
                }

                // Exists purely for cosmetic reasons to make the border of
                // optionsToolBar that meets annotationsToolBar look better
                Rectangle {
                    id: borderBg
                    z: -1
                    visible: otbLoader.visible
                    opacity: otbLoader.opacity
                    parent: annotationsToolBar
                    x: otbLoader.x + annotationsToolBar.background.border.width
                    y: (annotationsToolBar.valignment & Qt.AlignTop)
                        && !SelectionEditor.selection.empty
                        ? otbLoader.y + otbLoader.height : otbLoader.y
                    width: otbLoader.width - annotationsToolBar.background.border.width * 2
                    height: dprRound(1)
                    color: annotationsToolBar.background.color
                }

                HoverHandler {
                    enabled: dragHandler.enabled
                    target: annotationsToolBar
                    margin: annotationsToolBar.padding
                    cursorShape: enabled ?
                        (dragHandler.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor)
                        : undefined
                }
                DragHandler { // parented to contentItem
                    id: dragHandler
                    target: atbLoader
                    acceptedButtons: Qt.LeftButton
                    margin: annotationsToolBar.padding
                    xAxis.minimum: root.viewportRect.x
                    xAxis.maximum: root.viewportRect.x + root.width - annotationsToolBar.width
                    yAxis.minimum: root.viewportRect.y
                    yAxis.maximum: root.viewportRect.y + root.height - annotationsToolBar.height
                    onActiveChanged: if (active) {
                        mainToolBar.z = mainToolBar.z > ftbLoader.z ? 1 : 0
                        atbLoader.z = 2
                        ftbLoader.z = mainToolBar.z < ftbLoader.z ? 1 : 0
                        if (!annotationsToolBar.rememberPosition) {
                            annotationsToolBar.rememberPosition = true
                        }
                    }
                }

                AnimatedLoader {
                    id: otbLoader
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
                    y: (annotationsToolBar.valignment & Qt.AlignTop)
                        && !SelectionEditor.selection.empty
                        ? -otbLoader.height + borderBg.height
                        : otbLoader.height - borderBg.height
                    state: if (root.document?.tool.options !== AnnotationTool.NoOptions
                                || (root.document?.tool.type === AnnotationTool.SelectTool
                                    && root.document?.selectedItem.options !== AnnotationTool.NoOptions)
                    ) {
                        return "active"
                    } else {
                        return "inactive"
                    }
                    sourceComponent: FloatingToolBar {
                        focusPolicy: Qt.NoFocus
                        contentItem: AnnotationOptionsToolBarContents {
                            spacing: parent.spacing
                            displayMode: QQC.AbstractButton.IconOnly
                            focusPolicy: Qt.NoFocus
                        }
                        topLeftRadius: ((annotationsToolBar.valignment & Qt.AlignBottom) || SelectionEditor.selection.empty) && otbLoader.x >= 0 ? 0 : radius
                        topRightRadius: ((annotationsToolBar.valignment & Qt.AlignBottom) || SelectionEditor.selection.empty) && otbLoader.x + width <= annotationsToolBar.width ? 0 : radius
                        bottomLeftRadius: (annotationsToolBar.valignment & Qt.AlignTop) && !SelectionEditor.selection.empty && otbLoader.x >= 0 ? 0 : radius
                        bottomRightRadius: (annotationsToolBar.valignment & Qt.AlignTop) && !SelectionEditor.selection.empty && otbLoader.x + width <= annotationsToolBar.width ? 0 : radius
                    }
                }
            }
        }

        // Finalizer ToolBar
        Loader {
            id: ftbLoader
            readonly property rect rect: Qt.rect(x, y, width, height)
            visible: !SpectacleCore.videoPlatform.isRecording && !SelectionEditor.selection.empty
            active: visible
            sourceComponent: FloatingToolBar {
                id: toolBar
                property bool rememberPosition: false
                property alias dragging: dragHandler.active
                readonly property int valignment: {
                    if (SelectionEditor.screensRect.height - SelectionEditor.handlesRect.bottom >= height + toolBar.topPadding || SelectionEditor.selection.empty) {
                        // the bottom of the bottom side of the selection
                        // or the bottom of the screen
                        return Qt.AlignBottom
                    } else {
                        // the top of the bottom side of the selection
                        return Qt.AlignTop
                    }
                }
                Binding {
                    property: "x"
                    target: ftbLoader
                    value: {
                        const v = SelectionEditor.selection.empty ? (root.width - toolBar.width) / 2 + root.viewportRect.x
                                                    : SelectionEditor.selection.horizontalCenter - toolBar.width / 2
                        return Math.max(toolBar.leftPadding, // min value
                            Math.min(dprRound(v),
                                        SelectionEditor.screensRect.width - toolBar.width - toolBar.rightPadding)) // max value
                    }
                    when: screensRectItem.allowToolbars && !toolBar.rememberPosition
                    restoreMode: Binding.RestoreNone
                }
                Binding {
                    property: "y"
                    target: ftbLoader
                    value: {
                        let v = 0
                        if (SelectionEditor.selection.empty) {
                            v = root.viewportRect.y + root.height - toolBar.height - toolBar.bottomPadding
                        } else if (toolBar.valignment & Qt.AlignBottom) {
                            v = SelectionEditor.handlesRect.bottom + toolBar.topPadding
                        } else if (toolBar.valignment & Qt.AlignTop) {
                            v = Math.min(SelectionEditor.selection.bottom, SelectionEditor.handlesRect.bottom - Kirigami.Units.gridUnit)
                                - toolBar.height - toolBar.bottomPadding
                        } else {
                            v = (toolBar.height / 2) - toolBar.parent.y
                        }
                        return dprRound(v)
                    }
                    when: screensRectItem.allowToolbars && !toolBar.rememberPosition
                    restoreMode: Binding.RestoreNone
                }
                visible: opacity > 0
                opacity: screensRectItem.allowToolbars && Geometry.rectIntersects(ftbLoader.rect, root.viewportRect)
                Behavior on opacity {
                    NumberAnimation {
                        duration: Kirigami.Units.longDuration
                        easing.type: Easing.OutCubic
                    }
                }
                layer.enabled: true // improves the visuals of the opacity animation
                focusPolicy: Qt.NoFocus
                contentItem: Loader {
                    sourceComponent: SpectacleCore.videoMode ? videoToolBarComponent : imageFinalizerToolBarComponent
                }

                HoverHandler {
                    enabled: dragHandler.enabled
                    target: toolBar
                    margin: toolBar.padding
                    cursorShape: enabled ?
                        (dragHandler.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor)
                        : undefined
                }
                DragHandler { // parent is contentItem and parent is a read-only property
                    id: dragHandler
                    target: ftbLoader
                    acceptedButtons: Qt.LeftButton
                    margin: toolBar.padding
                    xAxis.minimum: root.viewportRect.x
                    xAxis.maximum: root.viewportRect.x + root.width - toolBar.width
                    yAxis.minimum: root.viewportRect.y
                    yAxis.maximum: root.viewportRect.y + root.height - toolBar.height
                    onActiveChanged: if (active) {
                        mainToolBar.z = mainToolBar.z > atbLoader.z ? 1 : 0
                        atbLoader.z = mainToolBar.z < atbLoader.z ? 1 : 0
                        ftbLoader.z = 2
                        if (!toolBar.rememberPosition) {
                            toolBar.rememberPosition = true
                        }
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
