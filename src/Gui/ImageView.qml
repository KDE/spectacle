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
    Keys.forwardTo: flickable

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

    // Can't use ScrollView because ScrollView prevents mice from being able to drag the view
    EmptyPage { // parent is contentItem
        id: contentPage
        anchors {
            left: footerLoader.left
            right: captureOptionsLoader.left
            top: inlineMessageLoader.bottom
            bottom: footerLoader.top
            topMargin: inlineMessageLoader.active ? Kirigami.Units.mediumSpacing : 0
        }
        leftPadding: mirrored && verticalScrollBar.visible ? verticalScrollBar.width : 0
        rightPadding: !mirrored && verticalScrollBar.visible ? verticalScrollBar.width : 0
        bottomPadding: horizontalScrollBar.visible ? horizontalScrollBar.height : 0
        contentItem: Flickable {
            id: flickable
            function zoomByFactor(factor, centerPos = mapToItem(contentItem, width/2, height/2)) {
                let newWidth = Math.max(width, // min
                               Math.min(annotationsParent.width * factor,
                                        annotationsParent.implicitWidth * annotationsParent.maxZoom)) // max
                let newHeight = Math.max(height, // min
                                Math.min(annotationsParent.height * factor,
                                         annotationsParent.implicitHeight * annotationsParent.maxZoom)) // max
                resizeContent(newWidth, newHeight, centerPos)
                returnToBounds()
                if (newWidth === width) {
                    contentWidth = -1 // reset to follow flickable width
                }
                if (newHeight === height) {
                    contentHeight = -1 // reset to follow flickable height
                }
            }

            function zoomToPercent(percent, centerPos = mapToItem(contentItem, width/2, height/2)) {
                let newWidth = Math.max(width, // min
                               Math.min(annotationsParent.item.implicitWidth * percent,
                                        annotationsParent.item.implicitWidth * annotationsParent.maxZoom)) // max
                let newHeight = Math.max(height, // min
                                Math.min(annotationsParent.item.implicitHeight * percent,
                                         annotationsParent.item.implicitHeight * annotationsParent.maxZoom)) // max
                resizeContent(newWidth, newHeight, centerPos)
                returnToBounds()
                if (newWidth === width) {
                    contentWidth = -1 // reset to follow flickable width
                }
                if (newHeight === height) {
                    contentHeight = -1 // reset to follow flickable height
                }
            }

            clip: contextWindow.annotating
            interactive: contextWindow.annotating
                && AnnotationDocument.tool.type === AnnotationDocument.None
            boundsBehavior: Flickable.StopAtBounds
            rebound: Transition {} // Instant transition. Null doesn't do this.

            // Needed to re-center the content when the window has been
            // resized to be larger than the content after zooming in.
            // Can't use contentWidth, contentHeight, contentX and contentY here for some reason.
            // Doing so causes the content to be positioned wrong or make the centering not work.
            Binding on contentItem.x {
                value: (flickable.width - flickable.contentItem.width) / 2
                when: flickable.width > flickable.contentItem.width
                restoreMode: Binding.RestoreBindingOrValue
            }
            Binding on contentItem.y {
                value: (flickable.height - flickable.contentItem.height) / 2
                when: flickable.height > flickable.contentItem.height
                restoreMode: Binding.RestoreBindingOrValue
            }

            Kirigami.WheelHandler {
                property point angleDelta: Qt.point(0,0)
                Binding on angleDelta { // reset when annotation tools are hidden
                    value: Qt.point(0,0)
                    when: !contextWindow.annotating
                    restoreMode: Binding.RestoreNone
                }
                target: flickable
                keyNavigationEnabled: true
                scrollFlickableTarget: contextWindow.annotating
                onWheel: if (wheel.modifiers & Qt.ControlModifier && scrollFlickableTarget) {
                    // apparently it's impossible to add points to each other directly in QML
                    angleDelta.x += wheel.angleDelta.x
                    angleDelta.y += wheel.angleDelta.y
                    if (angleDelta.x >= 120 || angleDelta.y >= 120) {
                        angleDelta = Qt.point(0,0)
                        const centerPos = flickable.mapToItem(flickable.contentItem, wheel.x, wheel.y)
                        flickable.zoomByFactor(1.25, centerPos)
                    } else if (angleDelta.x <= -120 || angleDelta.y <= -120) {
                        angleDelta = Qt.point(0,0)
                        const centerPos = flickable.mapToItem(flickable.contentItem, wheel.x, wheel.y)
                        flickable.zoomByFactor(0.8, centerPos)
                    }
                    wheel.accepted = true
                }
            }

            QQC2.ScrollBar.vertical: QQC2.ScrollBar {
                id: verticalScrollBar
                parent: contentPage
                z: 1
                anchors.right: parent.right
                y: contentPage.topPadding
                height: contentPage.availableHeight
                active: horizontalScrollBar.active
            }
            QQC2.ScrollBar.horizontal: QQC2.ScrollBar {
                id: horizontalScrollBar
                parent: contentPage
                z: 1
                x: contentPage.leftPadding
                anchors.bottom: parent.bottom
                width: contentPage.availableWidth
                active: verticalScrollBar.active
            }

            MouseArea {
                anchors.fill: parent
                z: -1
                enabled: flickable.interactive
                    && (flickable.contentWidth > flickable.width
                        || flickable.contentHeight > flickable.height)
                cursorShape: enabled ?
                    (pressed || flickable.dragging ? Qt.ClosedHandCursor : Qt.OpenHandCursor)
                    : undefined
            }

            MouseArea {
                anchors.fill: annotationsParent
                cursorShape: enabled ?
                    (pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor)
                    : undefined
                enabled: !contextWindow.annotating
                onPositionChanged: {
                    contextWindow.startDrag()
                }
            }

            Loader {
                id: annotationsParent
                x: contextWindow.dprRound((parent.width - width) / 2)
                y: contextWindow.dprRound((parent.height - height) / 2)
                readonly property real minZoom: Math.min(flickable.width / item.implicitWidth,
                                                         flickable.height / item.implicitHeight)
                readonly property real maxZoom: Math.max(minZoom, 8)
                readonly property real defaultZoom: Math.min(parent.width / item.implicitWidth,
                                                             parent.height / item.implicitHeight)
                width: item.implicitWidth * (item.zoom < 1 ? item.zoom : item.scale)
                height: item.implicitHeight * (item.zoom < 1 ? item.zoom : item.scale)

                source: SpectacleCore.videoMode ? "RecordingView.qml" : "ScreenshotView.qml"
            }
        }
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
                    from: annotationsParent.minZoom * 100
                    to: annotationsParent.maxZoom * 100
                    stepSize: 10
                    value: annotationsParent.item.effectiveZoom * 100
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
                    onValueModified: flickable.zoomToPercent(Math.round(value) / 100)
                }
            }
        }
    }

    FontMetrics {
        id: fontMetrics
    }

    Shortcut {
        enabled: contextWindow.annotating
        sequences: [StandardKey.ZoomIn]
        onActivated: flickable.zoomByFactor(1.25)
    }
    Shortcut {
        enabled: contextWindow.annotating
        sequences: [StandardKey.ZoomOut]
        onActivated: flickable.zoomByFactor(0.8)
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
            PropertyChanges {
                target: flickable
                contentWidth: annotationsParent.item.implicitWidth
                contentHeight: annotationsParent.item.implicitHeight
            }
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
            PropertyChanges {
                target: flickable
                contentWidth: -1
                contentHeight: -1
            }
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
                    value: !SpectacleCore.videoMode
                }
                ParallelAnimation {
                    AnchorAnimation {
                        duration: Kirigami.Units.longDuration
                        easing.type: Easing.OutCubic
                    }
                    NumberAnimation {
                        target: flickable
                        properties: "contentWidth,contentHeight"
                        duration: Kirigami.Units.longDuration
                        easing.type: Easing.InOutSine
                    }
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
                ParallelAnimation {
                    AnchorAnimation {
                        duration: Kirigami.Units.longDuration
                        easing.type: Easing.OutCubic
                    }
                    NumberAnimation {
                        target: flickable
                        properties: "contentWidth,contentHeight"
                        duration: Kirigami.Units.longDuration
                        easing.type: Easing.InOutSine
                    }
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
