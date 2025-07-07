/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls as QQC
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private
import org.kde.kquickimageeditor

/**
 * This page is shown when a screenshot has been taken
 * or accepted from the rectangular region capture mode.
 *
 * - There is a `contextWindow` context property that can be used to
 * access the instance of the ViewerWindow.
 */
EmptyPage {
    id: root
    focus: true

    // Used in ViewerWindow::setMode()
    readonly property real minimumWidth: Math.max(
        header.implicitWidth,
        annotationsToolBar.implicitWidth + separator.implicitWidth + footerLoader.implicitWidth,
        480 // leave some room for content if necessary
    )
    readonly property real minimumHeight: header.implicitHeight
        + Math.max(annotationsToolBar.implicitHeight,
                   footerLoader.implicitHeight)

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    header: QQC.ToolBar {
        id: header
        contentItem: ButtonGrid {
            id: mainToolBarContents
            animationsEnabled: true
            AnimatedLoader {
                state: contextWindow.annotating ? "active" : "inactive"
                sourceComponent: UndoRedoGroup {
                    buttonHeight: QmlUtils.iconTextButtonHeight
                    spacing: mainToolBarContents.spacing
                }
            }
            TtToolButton {
                display: TtToolButton.IconOnly
                visible: action.enabled
                action: SaveAction {}
            }
            TtToolButton {
                display: SpectacleCore.videoMode ? TtToolButton.TextBesideIcon : TtToolButton.IconOnly
                action: SaveAsAction {}
            }
            TtToolButton {
                display: TtToolButton.IconOnly
                visible: action.enabled
                action: CopyImageAction {}
            }
             
            TtToolButton {
                display: TtToolButton.IconOnly
                visible: !SpectacleCore.videoMode && SpectacleCore.ocrAvailable
                action: OcrAction {}
            }
             
            // We only show this in video mode to save space in screenshot mode
            TtToolButton {
                visible: SpectacleCore.videoMode
                action: CopyLocationAction {}
            }
            ExportMenuButton {}
            TtToolButton {
                visible: action.enabled
                action: EditAction {}
            }
            QQC.ToolSeparator {
                height: QmlUtils.iconTextButtonHeight
            }
            ScreenshotModeMenuButton {}
            RecordingModeMenuButton {}
            OptionsMenuButton {}
        }
    }

    // Needed for scrolling via keyboard input
    Keys.priority: Keys.AfterItem
    Keys.enabled: !SpectacleCore.videoMode && contentLoader.item !== null
    Keys.forwardTo: contentLoader.item

    InlineMessageList {
        id: inlineMessageList
        // parent is contentItem
        anchors.left: annotationsToolBar.right
        anchors.right: parent.right
        anchors.top: parent.top
    }

    QQC.Pane { // parent is contentItem
        id: annotationsToolBar
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.left
        visible: false
        leftPadding: header.leftPadding
        rightPadding: header.rightPadding
        topPadding: header.topPadding
        bottomPadding: header.bottomPadding
        contentItem: AnnotationsToolBarContents {
            id: annotationsToolBarContents
            displayMode: QQC.AbstractButton.IconOnly
            flow: Grid.TopToBottom
            showUndoRedo: false
            showNoneButton: true
        }
        background: Rectangle {
            color: parent.palette.window
        }
    }

    Kirigami.Separator { // parent is contentItem
        id: separator
        visible: false
        anchors.left: annotationsToolBar.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
    }

    Loader { // parent is contentItem
        id: contentLoader
        anchors {
            left: footerLoader.left
            right: parent.right
            top: inlineMessageList.bottom
            bottom: footerLoader.top
        }
        sourceComponent: SpectacleCore.videoMode ? recordingViewComponent : screenshotViewComponent
        Component {
            id: recordingViewComponent
            RecordingView {}
        }
        Component {
            id: screenshotViewComponent
            ScreenshotView { showCropTool: annotationsToolBarContents.usingCropTool }
        }
    }

    Loader {
        id: footerLoader
        anchors.left: separator.right
        anchors.right: parent.right
        anchors.top: parent.bottom
        visible: false
        active: visible
        sourceComponent: QQC.ToolBar { // parent is contentItem
            position: QQC.ToolBar.Footer
            contentHeight: QmlUtils.iconTextButtonHeight
            contentItem: RowLayout {
                spacing: Kirigami.Units.mediumSpacing
                AnimatedLoader {
                    id: loader
                    Layout.fillWidth: true
                    active: opacity > 0
                    visible: true
                    state: if (SpectacleCore.annotationDocument.tool.options !== AnnotationTool.NoOptions
                              || (SpectacleCore.annotationDocument.tool.type === AnnotationTool.SelectTool
                                 && SpectacleCore.annotationDocument.selectedItem.options !== AnnotationTool.NoOptions)
                    ) {
                        return "active"
                    } else {
                        return "inactive"
                    }
                    source: "AnnotationOptionsToolBarContents.qml"
                }
                QQC.ToolSeparator {
                    Layout.fillHeight: true
                    visible: loader.implicitWidth
                        + implicitWidth
                        + zoomLabel.implicitWidth
                        + zoomEditor.implicitWidth
                        + parent.spacing * 3 >= parent.width
                }
                QQC.Label {
                    id: zoomLabel
                    text: i18n("Zoom:")
                }
                QQC.SpinBox {
                    id: zoomEditor
                    from: contentLoader.item.minZoom * 100
                    to: contentLoader.item.maxZoom * 100
                    stepSize: 25
                    value: contentLoader.item.currentZoom * 100
                    textFromValue: (value, locale) => {
                        return Number(Math.round(value)).toLocaleString(locale, 'f', 0) + locale.percent
                    }
                    valueFromText: (text, locale) => {
                        return Number.fromLocaleString(locale, text.replace(/\D/g,''))
                    }
                    QQC.ToolTip.text: i18n("Image Zoom")
                    QQC.ToolTip.visible: hovered
                    QQC.ToolTip.delay: Kirigami.Units.toolTipDelay
                    Binding {
                        target: zoomEditor.contentItem
                        property: "horizontalAlignment"
                        value: Text.AlignRight
                        restoreMode: Binding.RestoreNone
                    }
                    TextContextMenuConnection {
                        target: zoomEditor.contentItem
                    }
                    onValueModified: contentLoader.item.zoomToPercent(Math.round(value) / 100)
                }
            }
        }
    }

    Shortcut {
        enabled: contextWindow.annotating && !SpectacleCore.videoMode && contentLoader.item !== null
        sequences: [StandardKey.ZoomIn]
        onActivated: contentLoader.item.zoomIn()
    }
    Shortcut {
        enabled: contextWindow.annotating && !SpectacleCore.videoMode && contentLoader.item !== null
        sequences: [StandardKey.ZoomOut]
        onActivated: contentLoader.item.zoomOut()
    }
    // FIXME: This shortcut only exists here because spectacle interprets "Ctrl+Shift+,"
    // as "Ctrl+Shift+<" for some reason unless we use a QML Shortcut.
    Shortcut {
        sequences: [StandardKey.Preferences]
        onActivated: contextWindow.showPreferencesDialog()
    }

    state: "normal"
    states: [
        State {
            name: "annotating"
            when: contextWindow.annotating
                && !SpectacleCore.videoMode
                && contentLoader.item !== null
            AnchorChanges {
                target: annotationsToolBar
                anchors.left: parent.left
                anchors.right: undefined
            }
            AnchorChanges {
                target: footerLoader
                anchors.bottom: parent.bottom
                anchors.top: undefined
            }
        },
        State {
            name: "normal"
            when: !contextWindow.annotating
                || SpectacleCore.videoMode
                || contentLoader.item === null
            AnchorChanges {
                target: annotationsToolBar
                anchors.left: undefined
                anchors.right: parent.left
            }
            AnchorChanges {
                target: footerLoader
                anchors.bottom: undefined
                anchors.top: parent.bottom
            }
        }
    ]
    transitions: [
        Transition {
            to: "annotating"
            SequentialAnimation {
                PropertyAction {
                    targets: [annotationsToolBar, separator, footerLoader]
                    property: "visible"
                    value: true
                }
                AnchorAnimation {
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.OutCubic
                }
            }
        },
        Transition {
            to: "normal"
            SequentialAnimation {
                AnchorAnimation {
                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.OutCubic
                }
                PropertyAction {
                    targets: [annotationsToolBar, separator, footerLoader]
                    property: "visible"
                    value: false
                }
            }
        }
    ]
}
