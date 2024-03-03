/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls as QQC
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

/**
 * This page is shown when a user does not want to take a screenshot when Spectacle is started.
 * It allows the user to set up screen capturing and export screen captures.
 *
 * - There is a `contextWindow` context property that can be used to
 * access the instance of the ViewerWindow.
 */
EmptyPage {
    id: root

    property var inlineMessageData: {}
    property string inlineMessageSource: ""
    onInlineMessageDataChanged: if (inlineMessageSource) {
        inlineMessageLoader.setSource(inlineMessageSource, inlineMessageData)
        inlineMessageLoader.state = "active"
    }

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    padding: Kirigami.Units.mediumSpacing * 4

    Item {
        parent: root
        anchors.fill: parent
        z: -1
        // Allow dragging the window from anywhere
        DragHandler {
            acceptedButtons: Qt.LeftButton
            target: null
            // BUG: https://bugreports.qt.io/browse/QTBUG-110145
            // Changing acceptedButtons cancels the drag when the left mouse button is released on
            // Wayland. If we don't do this, you need to click somewhere in the window to be able to
            // click or hover on UI elements again.
            onActiveChanged: if (active) {
                acceptedButtons = contextWindow.startSystemMove() ? Qt.NoButton : Qt.LeftButton
            } else {
                acceptedButtons = Qt.LeftButton
            }
        }
    }

    component ConfigHelpButtonRow : Row {
        spacing: Kirigami.Units.mediumSpacing
        QQC.Button {
            y: dprRound((parent.height - height) / 2)
            icon.name: "configure"
            text: i18n("Configure Spectacle…")
            onClicked: contextWindow.showPreferencesDialog()
        }
        // Only ToolButton supports the menu button style with qqc2-desktop-style
        QQC.ToolButton {
            y: dprRound((parent.height - height) / 2)
            flat: false
            icon.name: "help-contents"
            text: i18n("Help")
            down: pressed || HelpMenu.visible
            Accessible.role: Accessible.ButtonMenu
            onPressed: HelpMenu.popup(this)
        }
    }

    Component {
        id: screenshotComponent
        GridLayout {
            rowSpacing: Kirigami.Units.mediumSpacing
            columnSpacing: Kirigami.Units.mediumSpacing * 4
            columns: 2
            rows: 2
            flow: GridLayout.TopToBottom

            Kirigami.Heading {
                topPadding: -captureHeadingMetrics.descent
                bottomPadding: -captureHeadingMetrics.descent + parent.rowSpacing
                text: i18nc("@title:group", "Screenshot Modes")
                level: 3
                FontMetrics {
                    id: captureHeadingMetrics
                }
            }

            CaptureModeButtonsColumn {
                id: buttonsColumn
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            }

            Kirigami.Heading {
                topPadding: -captureHeadingMetrics.descent
                bottomPadding: -captureHeadingMetrics.descent + parent.rowSpacing
                text: i18nc("@title:group", "Screenshot Settings")
                level: 3
            }

            CaptureSettingsColumn {
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                Layout.preferredWidth: buttonRow.implicitWidth
                ConfigHelpButtonRow {
                    id: buttonRow
                }
            }
        }
    }

    Component {
        id: recordingComponent
        GridLayout {
            rowSpacing: Kirigami.Units.mediumSpacing
            columnSpacing: Kirigami.Units.mediumSpacing * 4
            columns: 2
            rows: 2
            flow: GridLayout.TopToBottom

            Kirigami.Heading {
                topPadding: -captureHeadingMetrics.descent
                bottomPadding: -captureHeadingMetrics.descent + parent.rowSpacing
                text: i18nc("@title:group", "Recording Modes")
                level: 3
                FontMetrics {
                    id: captureHeadingMetrics
                }
            }

            RecordingModeButtonsColumn {
                id: buttonsColumn
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            }

            Kirigami.Heading {
                topPadding: -captureHeadingMetrics.descent
                bottomPadding: -captureHeadingMetrics.descent + parent.rowSpacing
                text: i18nc("@title:group", "Recording Settings")
                level: 3
            }

            RecordingSettingsColumn {
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                Layout.preferredWidth: buttonRow.implicitWidth
                ConfigHelpButtonRow {
                    id: buttonRow
                }
            }
        }
    }

    // Not ColumnLayout because layouts don't work well with animations.
    contentItem: Column {
        spacing: Kirigami.Units.mediumSpacing * 2
        // inline message and heading are not in header because we want the same padding as content.
        AnimatedLoader {
            id: inlineMessageLoader
            state: "inactive"
            width: parent.width
            height: visible ? implicitHeight : 0
            Behavior on height {
                NumberAnimation {
                    duration: inlineMessageLoader.animationDuration
                    easing.type: Easing.OutCubic
                }
            }
        }
        Loader {
            width: Math.max(parent.width, implicitWidth)
            active: VideoPlatform.supportedRecordingModes
            visible: active
            sourceComponent: Row {
                // We don't want the visuals of a tab because they don't look nice here with
                // qqc2-desktop-style, but this is functionally a tab.
                // Maybe use a segmented control style in the future when one becomes available.
                QQC.ToolButton {
                    id: screenshotHeadingButton
                    Accessible.role: Accessible.PageTab
                    width: Math.max(parent.width / 2, dprRound(implicitWidth), dprRound(recordingHeadingButton.implicitWidth))
                    height: Math.max(parent.height, dprRound(implicitHeight), dprRound(recordingHeadingButton.implicitHeight))
                    // Extend the clickable area between the buttons
                    leftInset: !mirrored ? 0 : Kirigami.Units.mediumSpacing
                    rightInset: mirrored ? 0 : Kirigami.Units.mediumSpacing
                    padding: Kirigami.Units.mediumSpacing * 2
                    // Visually compensate for inset to make it look like spacing
                    leftPadding: padding + leftInset
                    rightPadding: padding + rightInset
                    // Make the padding look more even with Latin-like scripts.
                    // There should still be more than enough space for other scripts.
                    topPadding: Math.max(0, padding - headingFontMetrics.descent)
                    bottomPadding: Math.max(0, padding - headingFontMetrics.descent)
                    // We do `pointSize * sqrt(2)` because `pointSize * 2` would actually result
                    // in a font that uses 4x more area than the base font would have.
                    font.pointSize: Application.font.pointSize * 1.414213562373095
                    text: i18nc("@title:tab", "Screenshot")
                    checkable: true
                    checked: true
                    autoExclusive: true
                    FontMetrics {
                        id: headingFontMetrics
                        font: screenshotHeadingButton.font
                    }
                    onCheckedChanged: if (checked && contentLoader.sourceComponent !== screenshotComponent) {
                        contentLoader.sourceComponent = screenshotComponent
                    }
                }

                QQC.ToolButton {
                    id: recordingHeadingButton
                    Accessible.role: screenshotHeadingButton.Accessible.role
                    width: screenshotHeadingButton.width
                    height: screenshotHeadingButton.height
                    leftInset: !mirrored ? Kirigami.Units.mediumSpacing : 0
                    rightInset: mirrored ? Kirigami.Units.mediumSpacing : 0
                    padding: screenshotHeadingButton.padding
                    leftPadding: padding + leftInset
                    rightPadding: padding + rightInset
                    topPadding: screenshotHeadingButton.topPadding
                    bottomPadding: screenshotHeadingButton.bottomPadding
                    font: screenshotHeadingButton.font
                    text: i18nc("@title:tab", "Recording")
                    checkable: true
                    checked: false
                    autoExclusive: true
                    onCheckedChanged: if (checked && contentLoader.sourceComponent !== recordingComponent) {
                        contentLoader.sourceComponent = recordingComponent
                    }
                }
            }
        }
        Loader {
            id: contentLoader
            width: Math.max(parent.width, implicitWidth)
            sourceComponent: screenshotComponent
        }
    }

    // FIXME: This shortcut only exists here because spectacle interprets "Ctrl+Shift+,"
    // as "Ctrl+Shift+<" for some reason unless we use a QML Shortcut.
    Shortcut {
        sequences: [StandardKey.Preferences]
        onActivated: contextWindow.showPreferencesDialog()
    }
}
