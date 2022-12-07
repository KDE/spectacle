/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Templates 2.15 as T
import org.kde.kirigami 2.19 as Kirigami
import org.kde.spectacle.private 1.0

/**
 * This page is shown when a user does not want to take a screenshot when Spectacle is started.
 * It allows the user to set up screen capturing and export screen captures.
 *
 * - There is a `contextWindow` context property that can be used to
 * access the instance of the ViewerWindow.
 */
EmptyPage {
    id: root

    property var inlineMessageData: null
    onInlineMessageDataChanged: {
        inlineMessageLoader.setSource(inlineMessageData[0],
                                      {"messageArgument": inlineMessageData[1]})
        inlineMessageLoader.state = "active"
    }

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    padding: Kirigami.Units.mediumSpacing * 4
    topPadding: 0

    header: Item {
        implicitWidth: Math.max(inlineMessageLoader.implicitWidth
                                + Kirigami.Units.mediumSpacing * 2,
                                contextWindow.dprRound(headerLabel.implicitWidth))
        implicitHeight: Math.max(inlineMessageLoader.implicitHeight
                                 + Kirigami.Units.mediumSpacing * 2,
                                 contextWindow.dprRound(headerLabel.implicitHeight))

        QQC2.Label {
            id: headerLabel
            visible: !inlineMessageLoader.visible
            anchors.fill: parent
            padding: Kirigami.Units.mediumSpacing * 4
            topPadding: padding - headingFontMetrics.descent
            bottomPadding: topPadding
            font.pixelSize: fontMetrics.height
            text: i18n("Take a new screenshot")
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            FontMetrics {
                id: headingFontMetrics
            }
        }

        AnimatedLoader {
            id: inlineMessageLoader
            anchors.centerIn: parent
            state: "inactive"
        }

        height: implicitHeight
        Behavior on height {
            NumberAnimation {
                duration: inlineMessageLoader.animationDuration
                easing.type: Easing.OutCubic
            }
        }

        // This area is mostly blank space most of the time.
        // Let's make it a bit more useful by making it easier to move the window around.
        DragHandler {
            acceptedButtons: Qt.LeftButton
            dragThreshold: 0
            target: null
            onActiveChanged: if (active) {
                contextWindow.startSystemMove()
            }
        }
    }

    contentItem: Row {
        spacing: Kirigami.Units.mediumSpacing * 4
        Column {
            height: Math.max(implicitHeight, parent.height)
            spacing: Kirigami.Units.mediumSpacing
            QQC2.Label {
                height: fontMetrics.height + topPadding + bottomPadding
                topPadding: -fontMetrics.descent
                verticalAlignment: Text.AlignTop
                text: i18n("Capture Modes")
                font.bold: true
            }
            Repeater {
                model: SpectacleCore.captureModeModel
                delegate: QQC2.DelayButton {
                    id: button
                    readonly property bool showCancel: Settings.captureMode === model.captureMode && SpectacleCore.captureTimeRemaining > 0
                    width: Math.max(implicitWidth, parent.width)
                    leftPadding: Kirigami.Units.mediumSpacing + fontMetrics.descent
                    rightPadding: Kirigami.Units.mediumSpacing + fontMetrics.descent
                    topPadding: Kirigami.Units.mediumSpacing
                    bottomPadding: Kirigami.Units.mediumSpacing
                    // Delay doesn't really matter since we set
                    // progress directly and have no transition
                    delay: 1
                    transition: null
                    progress: Settings.captureMode === model.captureMode ?
                        SpectacleCore.captureProgress : 0
                    icon.name: showCancel ? "dialog-cancel" : ""
                    text: showCancel ?
                        i18np("Cancel (%1 second)", "Cancel (%1 seconds)",
                              Math.ceil(SpectacleCore.captureTimeRemaining / 1000))
                        : model.display
                    QQC2.ToolTip.text: model.shortcuts
                    QQC2.ToolTip.visible: (hovered || pressed) && model.shortcuts.length > 0
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    onClicked: if (showCancel) {
                        SpectacleCore.cancelScreenshot()
                    } else {
                        Settings.captureMode = model.captureMode
                        SpectacleCore.takeNewScreenshot()
                    }
                }
            }
        }
        Column {
            height: Math.max(implicitHeight, parent.height)
            spacing: Kirigami.Units.mediumSpacing
            // Label ToolButton ToolButton
            QQC2.Label {
                height: fontMetrics.height + topPadding + bottomPadding
                topPadding: -fontMetrics.descent
                verticalAlignment: Text.AlignTop
                text: i18n("Capture Settings")
                font.bold: true
            }
            // Label SpinBox CheckBox
            RowLayout {
                width: Math.max(implicitWidth, parent.width)
                spacing: parent.spacing
                QQC2.Label {
                    text: i18n("Delay:")
                }
                DelaySpinBox {
                    Layout.fillWidth: true
                    enabled: !captureOnClickCheckBox.checked
                }
                QQC2.CheckBox {
                    id: captureOnClickCheckBox
                    text: i18n("On Click")
                    enabled: Platform.supportedShutterModes === Platform.Immediate | Platform.OnClick
                    QQC2.ToolTip.text: i18n("Wait for a mouse click before capturing the screenshot image")
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    QQC2.ToolTip.visible: hovered || pressed
                    checked: Platform.supportedShutterModes & Platform.OnClick && Settings.captureOnClick
                    onToggled: Settings.captureOnClick = checked
                }
            }
            // column of CheckBoxes
            QQC2.CheckBox {
                text: i18n("Include mouse pointer")
                QQC2.ToolTip.text: i18n("Show the mouse cursor in the screenshot image")
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                QQC2.ToolTip.visible: hovered
                checked: Settings.includePointer
                onToggled: Settings.includePointer = checked
            }
            QQC2.CheckBox {
                text: i18n("Include window titlebar and borders")
                QQC2.ToolTip.text: i18n("Show the window title bar, the minimize/maximize/close buttons, and the window border")
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                QQC2.ToolTip.visible: hovered
                checked: Settings.includeDecorations
                onToggled: Settings.includeDecorations = checked
            }
            QQC2.CheckBox {
                text: i18n("Capture the current pop-up only")
                QQC2.ToolTip.text: i18n("Capture only the current pop-up window (like a menu, tooltip etc).\n"
                                        + "If disabled, the pop-up is captured along with the parent window")
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                QQC2.ToolTip.visible: hovered
                checked: Settings.transientOnly
                onToggled: Settings.transientOnly = checked
            }
            QQC2.CheckBox {
                text: i18n("Quit after manual Save or Copy")
                QQC2.ToolTip.text: i18n("Quit Spectacle after manually saving or copying the image")
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                QQC2.ToolTip.visible: hovered
                checked: Settings.quitAfterSaveCopyExport
                onToggled: Settings.quitAfterSaveCopyExport = checked
            }
            // Button ToolButton
            Row {
                width: Math.max(implicitWidth, parent.width)
                spacing: parent.spacing
                QQC2.Button {
                    icon.name: "configure"
                    text: i18n("Configure Spectacle…")
                    onClicked: contextWindow.showPreferencesDialog()
                }
                QQC2.ToolButton {
                    flat: false
                    icon.name: "help-contents"
                    text: i18n("Help")
                    down: pressed || contextWindow.helpMenu.visible
                    onPressed: contextWindow.helpMenu.popup(mapToGlobal(0, height))
                    // NOTE: only qqc2-desktop-style and qqc2-breeze-style have showMenuArrow
                    Component.onCompleted: if (background.hasOwnProperty("showMenuArrow")) {
                        background.showMenuArrow = true
                    }
                }
            }
        }
    }

    FontMetrics {
        id: fontMetrics
    }

    // FIXME: This shortcut only exists here because spectacle interprets "Ctrl+Shift+,"
    // as "Ctrl+Shift+<" for some reason unless we use a QML Shortcut.
    Shortcut {
        sequences: [StandardKey.Preferences]
        onActivated: contextWindow.showPreferencesDialog()
    }
}
