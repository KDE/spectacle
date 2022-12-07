/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQml 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami
import org.kde.spectacle.private 1.0

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
        enabled: mainToolBarContents.annotationsButtonChecked && AnnotationDocument.tool.type !== AnnotationDocument.None
        viewportRect: Qt.rect(contextWindow.x, contextWindow.y, width, height)
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
        enabled: !annotations.enabled
        color: "transparent"
        border.color: palette.highlight
        border.width: contextWindow.dprRound(1)
        visible: !Selection.empty && Selection.rectIntersectsRect(Qt.rect(x,y,width,height),
                                                                  Qt.rect(0,0,parent.width, parent.height))
        x: Selection.x - border.width - contextWindow.x
        y: Selection.y - border.width - contextWindow.y
        width: Selection.width + border.width * 2
        height: Selection.height + border.width * 2

        LayoutMirroring.enabled: false
        LayoutMirroring.childrenInherit: true

        SystemPalette {
            id: palette
            colorGroup: selectionRectangle.enabled ? SystemPalette.Active : SystemPalette.Disabled
        }
    }

    Item {
        x: -contextWindow.x
        y: -contextWindow.y
        enabled: selectionRectangle.enabled
        component Handle: Rectangle {
            visible: enabled && selectionRectangle.visible
                && SelectionEditor.dragLocation === SelectionEditor.None
                && Selection.rectIntersectsRect(Qt.rect(x,y,width,height), annotations.viewportRect)
            color: palette.highlight
            width: Kirigami.Units.gridUnit
            height: width
            radius: width / 2
        }

        Handle {
            x: SelectionEditor.handlesRect.x
            y: SelectionEditor.handlesRect.y
        }
        Handle {
            x: SelectionEditor.handlesRect.x
            y: SelectionEditor.handlesRect.y + SelectionEditor.handlesRect.height/2 - height/2
        }
        Handle {
            x: SelectionEditor.handlesRect.x
            y: SelectionEditor.handlesRect.y + SelectionEditor.handlesRect.height - height
        }
        Handle {
            x: SelectionEditor.handlesRect.x + SelectionEditor.handlesRect.width/2 - width/2
            y: SelectionEditor.handlesRect.y
        }
        Handle {
            x: SelectionEditor.handlesRect.x + SelectionEditor.handlesRect.width/2 - width/2
            y: SelectionEditor.handlesRect.y + SelectionEditor.handlesRect.height - height
        }
        Handle {
            x: SelectionEditor.handlesRect.x + SelectionEditor.handlesRect.width - width
            y: SelectionEditor.handlesRect.y + SelectionEditor.handlesRect.height/2 - height/2
        }
        Handle {
            x: SelectionEditor.handlesRect.x + SelectionEditor.handlesRect.width - width
            y: SelectionEditor.handlesRect.y
        }
        Handle {
            x: SelectionEditor.handlesRect.x + SelectionEditor.handlesRect.width - width
            y: SelectionEditor.handlesRect.y + SelectionEditor.handlesRect.height - height
        }
    }

    FontMetrics {
        id: fontMetrics
    }

    ShortcutsTextBox {
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
        }
        visible: opacity > 0
        // Assume SelectionEditor covers all screens.
        // Use parent's coordinate system.
        opacity: root.containsMouse
            && !Selection.rectIntersectsRect(SelectionEditor.handlesRect,
                                             Qt.rect(x, y, width, height))
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
        x: -contextWindow.x
        y: -contextWindow.y

        // Magnifier
        Loader {
            x: Math.round(Math.min(SelectionEditor.mousePosition.x + Kirigami.Units.gridUnit , SelectionEditor.screensRect.width - width))
            y: Math.round(Math.min(SelectionEditor.mousePosition.y + Kirigami.Units.gridUnit , SelectionEditor.screensRect.height - height))
            z: 100
            visible: Settings.showMagnifier && SelectionEditor.magnifierAllowed
                && Selection.rectIntersectsRect(Qt.rect(x,y,width,height), annotations.viewportRect)
            active: Settings.showMagnifier
            sourceComponent: Magnifier {
                viewport: annotations
                targetPoint: SelectionEditor.mousePosition
            }
        }

        // Size ToolTip
        SizeLabel {
            id: ssToolTip
            readonly property int valignment: {
                if (Selection.empty) {
                    return 0
                }
                const margin = Kirigami.Units.mediumSpacing * 2
                const w = width + margin
                const h = height + margin
                if (SelectionEditor.handlesRect.top >= h) {
                    return Qt.AlignTop
                } else if (SelectionEditor.screensRect.height - SelectionEditor.handlesRect.bottom >= h) {
                    return Qt.AlignBottom
                } else {
                    return Qt.AlignVCenter
                }
            }
            Binding on x {
                value: contextWindow.dprRound(Selection.horizontalCenter - ssToolTip.width / 2)
                when: !Selection.empty
                restoreMode: Binding.RestoreNone
            }
            Binding on y {
                value: {
                    let v = 0
                    if (ssToolTip.valignment === Qt.AlignVCenter) {
                        v = Selection.verticalCenter - ssToolTip.height / 2
                    } else if (ssToolTip.valignment === Qt.AlignTop) {
                        v = SelectionEditor.handlesRect.top
                            - ssToolTip.height - Kirigami.Units.mediumSpacing * 2
                    } else if (ssToolTip.valignment === Qt.AlignBottom) {
                        v = SelectionEditor.handlesRect.bottom + Kirigami.Units.mediumSpacing * 2
                    }
                    return contextWindow.dprRound(v)
                }
                when: !Selection.empty
                restoreMode: Binding.RestoreNone
            }
            visible: opacity > 0
            opacity: !Selection.empty
                && !(mainToolBar.visible && mainToolBar.valignment === ssToolTip.valignment)
                && Selection.rectIntersectsRect(Qt.rect(x,y,width,height), annotations.viewportRect)
            Behavior on opacity {
                NumberAnimation {
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.OutCubic
                }
            }
            size: Selection.alignedSize(Selection.width, Selection.height,
                                        SelectionEditor.devicePixelRatio) // TODO: real pixel size on wayland
            padding: Kirigami.Units.mediumSpacing * 2
            topPadding: padding - fontMetrics.descent
            bottomPadding: topPadding
            background: FloatingBackground {
                implicitWidth: Math.ceil(parent.contentWidth) + parent.leftPadding + parent.rightPadding
                implicitHeight: Math.ceil(parent.contentHeight) + parent.topPadding + parent.bottomPadding
                color: Qt.rgba(parent.palette.window.r,
                            parent.palette.window.g,
                            parent.palette.window.b, 0.85)
                border.color: Qt.rgba(parent.palette.windowText.r,
                                    parent.palette.windowText.g,
                                    parent.palette.windowText.b, 0.2)
                border.width: contextWindow.dprRound(1)
            }
        }

        // Main ToolBar
        FloatingToolBar {
            id: mainToolBar
            readonly property int valignment: {
                if (height + topPadding
                    <= SelectionEditor.screensRect.height - SelectionEditor.handlesRect.bottom
                ) {
                    return Qt.AlignBottom
                } else if (height + bottomPadding
                    <= SelectionEditor.handlesRect.top
                ) {
                    return Qt.AlignTop
                } else {
                    return Qt.AlignVCenter
                }
            }
            Binding on x {
                value: Math.max(mainToolBar.leftPadding, // min value
                    Math.min(contextWindow.dprRound(Selection.horizontalCenter - mainToolBar.width / 2),
                                SelectionEditor.screensRect.width - mainToolBar.width - mainToolBar.rightPadding)) // max value
                when: !Selection.empty
                restoreMode: Binding.RestoreNone
            }
            Binding on y {
                value: {
                    let v = 0
                    // put above selection if not enough room below selection
                    if (mainToolBar.valignment === Qt.AlignTop) {
                        v = SelectionEditor.handlesRect.top - mainToolBar.height - mainToolBar.bottomPadding
                    } else if (mainToolBar.valignment === Qt.AlignBottom) {
                        v = SelectionEditor.handlesRect.bottom + mainToolBar.topPadding
                    } else {
                        v = Selection.verticalCenter - mainToolBar.height / 2
                    }
                    return contextWindow.dprRound(v)
                }
                when: !Selection.empty
                restoreMode: Binding.RestoreNone
            }
            visible: opacity > 0
            opacity: !Selection.empty && !SelectionEditor.dragLocation
                && Selection.rectIntersectsRect(Qt.rect(x,y,width,height), annotations.viewportRect)
            Behavior on opacity {
                NumberAnimation {
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.OutCubic
                }
            }
            layer.enabled: true // improves the visuals of the opacity animation
            focusPolicy: Qt.NoFocus
            contentItem: MainToolBarContents {
                id: mainToolBarContents
                focusPolicy: Qt.NoFocus
                annotationsButtonChecked: true
                displayMode: QQC2.AbstractButton.TextBesideIcon
                showSizeLabel: mainToolBar.valignment === ssToolTip.valignment
                imageSize: Selection.alignedSize(Selection.width,
                                                Selection.height,
                                                SelectionEditor.devicePixelRatio)
            }
        }

        AnimatedLoader {
            id: atbLoader
            readonly property int valignment: mainToolBar.valignment === Qt.AlignTop ?
                Qt.AlignTop : Qt.AlignBottom
            active: visible && mainToolBar.visible
            state: !Selection.empty && !SelectionEditor.dragLocation
                && mainToolBarContents.annotationsButtonChecked ? "active" : "inactive"
            x: Math.max(mainToolBar.x, // min value
               Math.min(contextWindow.dprRound(mainToolBar.x + (mainToolBar.width - width) / 2),
                        mainToolBar.x + mainToolBar.width - width)) // max value
            y: contextWindow.dprRound(valignment === Qt.AlignTop ?
                mainToolBar.y - height - Kirigami.Units.mediumSpacing
                : mainToolBar.y + mainToolBar.height + Kirigami.Units.mediumSpacing)
            sourceComponent: FloatingToolBar {
                id: annotationsToolBar
                focusPolicy: Qt.NoFocus
                contentItem: AnnotationsToolBarContents {
                    id: annotationsContents
                    displayMode: QQC2.AbstractButton.IconOnly
                    focusPolicy: Qt.NoFocus
                }

                topLeftRadius: optionsToolBar.visible
                    && optionsToolBar.x === 0
                    && atbLoader.valignment === Qt.AlignTop ? 0 : radius
                topRightRadius: optionsToolBar.visible
                    && optionsToolBar.x === width - optionsToolBar.width
                    && atbLoader.valignment === Qt.AlignTop ? 0 : radius
                bottomLeftRadius: optionsToolBar.visible
                    && optionsToolBar.x === 0
                    && atbLoader.valignment === Qt.AlignBottom ? 0 : radius
                bottomRightRadius: optionsToolBar.visible
                    && optionsToolBar.x === width - optionsToolBar.width
                    && atbLoader.valignment === Qt.AlignBottom ? 0 : radius

                // Exists purely for cosmetic reasons to make the border of
                // optionsToolBar that meets annotationsToolBar look better
                Rectangle {
                    id: borderBg
                    z: -1
                    visible: optionsToolBar.visible
                    opacity: optionsToolBar.opacity
                    parent: annotationsToolBar
                    x: optionsToolBar.x + annotationsToolBar.background.border.width
                    y: atbLoader.valignment === Qt.AlignTop ?
                        optionsToolBar.y + optionsToolBar.height : optionsToolBar.y
                    width: optionsToolBar.width - annotationsToolBar.background.border.width * 2
                    height: contextWindow.dprRound(1)
                    color: annotationsToolBar.background.color
                }

                AnimatedLoader {
                    id: optionsToolBar
                    parent: annotationsToolBar
                    x: {
                        const targetX = annotationsContents.x
                            + annotationsContents.checkedButton.x
                            + (annotationsContents.checkedButton.width - width) / 2
                        return Math.max(0, // min value
                               Math.min(contextWindow.dprRound(targetX),
                                        parent.width - width)) // max value
                    }
                    y: atbLoader.valignment === Qt.AlignTop ?
                        -optionsToolBar.height + borderBg.height
                        : optionsToolBar.height - borderBg.height
                    state: if (AnnotationDocument.tool.options !== AnnotationTool.NoOptions
                        || (AnnotationDocument.tool.type === AnnotationDocument.ChangeAction
                            && AnnotationDocument.selectedAction.options !== AnnotationTool.NoOptions)
                    ) {
                        return "active"
                    } else {
                        return "inactive"
                    }
                    sourceComponent: FloatingToolBar {
                        focusPolicy: Qt.NoFocus
                        contentItem: AnnotationOptionsToolBarContents {
                            displayMode: QQC2.AbstractButton.IconOnly
                            focusPolicy: Qt.NoFocus
                        }
                        topLeftRadius: atbLoader.valignment === Qt.AlignBottom && x >= 0 ? 0 : radius
                        topRightRadius: atbLoader.valignment === Qt.AlignBottom && x + width <= annotationsToolBar.width ? 0 : radius
                        bottomLeftRadius: atbLoader.valignment === Qt.AlignTop && x >= 0 ? 0 : radius
                        bottomRightRadius: atbLoader.valignment === Qt.AlignTop && x + width <= annotationsToolBar.width ? 0 : radius
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
}
