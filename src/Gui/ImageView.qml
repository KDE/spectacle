/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQml 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami
import org.kde.spectacle.private 1.0

import "Annotations"

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

        annotationsToolBar.implicitWidth + separator.implicitWidth + footerLoader.implicitWidth
    )
    readonly property real minimumHeight: header.implicitHeight
        + Math.max(annotationsToolBar.implicitHeight,
                   footerLoader.implicitHeight,
                   captureOptionsLoader.implicitHeight)

    property var inlineMessageData: {}
    property string inlineMessageSource: ""
    onInlineMessageDataChanged: {
        if (inlineMessageSource) {
            inlineMessageLoader.setSource(inlineMessageSource, inlineMessageData)
            inlineMessageLoader.state = "active"
        }
    }

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    header: QQC2.ToolBar {
        id: header
        contentItem: MainToolBarContents {
            id: mainToolBarContents
            showNewScreenshotButton: false
            showOptionsMenu: false
            showUndoRedo: contextWindow.annotating
            displayMode: QQC2.AbstractButton.TextBesideIcon
        }
    }

    // Needed for scrolling via keyboard input
    Keys.priority: Keys.AfterItem
    Keys.enabled: !SpectacleCore.videoMode && contentLoader.item !== null
    Keys.forwardTo: contentLoader.item

    AnimatedLoader { // parent is contentItem
        id: inlineMessageLoader
        anchors.left: annotationsToolBar.right
        anchors.right: captureOptionsLoader.left
        anchors.top: parent.top
        anchors.margins: visible ? Kirigami.Units.mediumSpacing : 0
        state: "inactive"
        height: visible ? implicitHeight : 0
        Behavior on height {
            NumberAnimation {
                duration: inlineMessageLoader.animationDuration
                easing.type: Easing.OutCubic
            }
        }
    }

    QQC2.Pane { // parent is contentItem
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
            displayMode: QQC2.AbstractButton.IconOnly
            flow: Grid.TopToBottom
            showUndoRedo: false
            rememberToolType: true
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
            right: captureOptionsLoader.left
            top: inlineMessageLoader.bottom
            bottom: footerLoader.top
            topMargin: inlineMessageLoader.active ? Kirigami.Units.mediumSpacing : 0
        }
        source: SpectacleCore.videoMode ? "RecordingView.qml" : "ScreenshotView.qml"
    }

    Loader { // parent is contentItem
        id: captureOptionsLoader
        visible: true
        active: visible
        anchors {
            top: parent.top
            bottom: parent.bottom
            right: parent.right
        }
        width: Math.min(parent.width/3, Kirigami.Units.gridUnit * 15)
        sourceComponent: QQC2.Page {

            leftPadding: Kirigami.Units.mediumSpacing * 2
                + (!mirrored ? sideBarSeparator.implicitWidth : 0)
            rightPadding: Kirigami.Units.mediumSpacing * 2
                + (mirrored ? sideBarSeparator.implicitWidth : 0)
            topPadding: Kirigami.Units.mediumSpacing * 2
            bottomPadding: Kirigami.Units.mediumSpacing * 2

            header: QQC2.TabBar {
                id: tabBar
                visible: SpectacleCore.recordingSupported
                currentIndex: 0
                QQC2.TabButton {
                    width: tabBar.width / tabBar.count
                    text: i18n("Screenshot")
                }
                QQC2.TabButton {
                    width: tabBar.width / tabBar.count
                    text: i18n("Recording")
                }
            }

            contentItem: Loader {
                source: switch (tabBar.currentIndex) {
                    case 0: return "CaptureOptions.qml"
                    case 1: return "RecordOptions.qml"
                    default: return ""
                }
            }

            background: Rectangle {
                color: Kirigami.Theme.backgroundColor
                Kirigami.Separator {
                    id: sideBarSeparator
                    anchors {
                        left: parent.left
                        top: parent.top
                        bottom: parent.bottom
                    }
                }
            }
        }
    }

    Loader {
        id: footerLoader
        anchors.left: separator.right
        anchors.right: captureOptionsLoader.left
        anchors.top: parent.bottom
        visible: false
        active: visible
        sourceComponent: QQC2.ToolBar { // parent is contentItem
            position: QQC2.ToolBar.Footer
            contentHeight: mainToolBarContents.fullButtonHeight
            contentItem: RowLayout {
                spacing: Kirigami.Units.mediumSpacing
                AnimatedLoader {
                    id: loader
                    Layout.fillWidth: true
                    active: opacity > 0
                    visible: true
                    state: if (AnnotationDocument.tool.options !== AnnotationTool.NoOptions
                        || (AnnotationDocument.tool.type === AnnotationDocument.ChangeAction
                            && AnnotationDocument.selectedAction.options !== AnnotationTool.NoOptions)
                    ) {
                        return "active"
                    } else {
                        return "inactive"
                    }
                    source: "AnnotationOptionsToolBarContents.qml"
                }
                QQC2.ToolSeparator {
                    Layout.fillHeight: true
                    visible: loader.implicitWidth
                        + implicitWidth
                        + zoomLabel.implicitWidth
                        + zoomEditor.implicitWidth
                        + parent.spacing * 3 >= parent.width
                }
                QQC2.Label {
                    id: zoomLabel
                    text: i18n("Zoom:")
                }
                QQC2.SpinBox {
                    id: zoomEditor
                    from: contentLoader.item.minZoom * 100
                    to: contentLoader.item.maxZoom * 100
                    stepSize: 10
                    value: contentLoader.item.effectiveZoom * 100
                    textFromValue: (value, locale) => {
                        return Number(Math.round(value)).toLocaleString(locale, 'f', 0) + locale.percent
                    }
                    valueFromText: (text, locale) => {
                        return Number.fromLocaleString(locale, text.replace(/\D/g,''))
                    }
                    QQC2.ToolTip.text: i18n("Image Zoom")
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    Binding {
                        target: zoomEditor.contentItem
                        property: "horizontalAlignment"
                        value: Text.AlignRight
                        restoreMode: Binding.RestoreNone
                    }
                    onValueModified: contentLoader.item.zoomToPercent(Math.round(value) / 100)
                }
            }
        }
    }

    FontMetrics {
        id: fontMetrics
    }

    Shortcut {
        enabled: contextWindow.annotating && !SpectacleCore.videoMode && contentLoader.item !== null
        sequences: [StandardKey.ZoomIn]
        onActivated: contentLoader.item.zoomToPercent(contentLoader.item.effectiveZoom * 1.25)
    }
    Shortcut {
        enabled: contextWindow.annotating && !SpectacleCore.videoMode && contentLoader.item !== null
        sequences: [StandardKey.ZoomOut]
        onActivated: contentLoader.item.zoomToPercent(contentLoader.item.effectiveZoom * 0.8)
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
            AnchorChanges {
                target: captureOptionsLoader
                anchors.left: parent.right
                anchors.right: undefined
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
            AnchorChanges {
                target: captureOptionsLoader
                anchors.left: undefined
                anchors.right: parent.right
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
                PropertyAction {
                    targets: captureOptionsLoader
                    property: "visible"
                    value: false
                }
            }
        },
        Transition {
            to: "normal"
            SequentialAnimation {
                PropertyAction {
                    targets: captureOptionsLoader
                    property: "visible"
                    value: true
                }
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
