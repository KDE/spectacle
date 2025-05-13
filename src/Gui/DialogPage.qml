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
                text: i18nc("@title:group", "Take a Screenshot:")
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
                text: i18nc("@title:group", "Options")
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
                text: i18nc("@title:group", "Make a Recording:")
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
                text: i18nc("@title:group", "Options")
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
    header: Column {
        spacing: 0

        Kirigami.NavigationTabBar {
            id: tabBar

            width: Math.max(parent.width, implicitWidth)
            visible: SpectacleCore.videoPlatform.supportedRecordingModes
            currentIndex: 0

            actions: [
                Kirigami.Action {
                    text: i18nc("@title:tab", "Screenshot")
                    icon.name: "camera-photo"
                    checked: tabBar.currentIndex === 0
                },
                Kirigami.Action {
                    text: i18nc("@title:tab", "Recording")
                    icon.name: "camera-video"
                    checked: tabBar.currentIndex === 1
                }
            ]
        }

        InlineMessageList {
            id: inlineMessageList
            width: parent.width
        }
    }

    contentItem: Loader {
        id: contentLoader
        sourceComponent: switch (tabBar.currentIndex) {
            case 0: return screenshotComponent
            case 1: return recordingComponent
            default: return null
        }
    }

    // FIXME: This shortcut only exists here because spectacle interprets "Ctrl+Shift+,"
    // as "Ctrl+Shift+<" for some reason unless we use a QML Shortcut.
    Shortcut {
        sequences: [StandardKey.Preferences]
        onActivated: contextWindow.showPreferencesDialog()
    }
}
