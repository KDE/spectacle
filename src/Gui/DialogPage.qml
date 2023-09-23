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
    topPadding: 0

    header: Item {
        implicitWidth: Math.max(inlineMessageLoader.implicitWidth
                                + Kirigami.Units.mediumSpacing * 2,
                                contextWindow.dprRound(headerLabel.implicitWidth))
        implicitHeight: Math.max(inlineMessageLoader.implicitHeight
                                 + Kirigami.Units.mediumSpacing * 2,
                                 contextWindow.dprRound(headerLabel.implicitHeight))

        QQC.Label {
            id: headerLabel
            visible: !inlineMessageLoader.visible
            anchors.fill: parent
            padding: Kirigami.Units.mediumSpacing * 4
            topPadding: padding - headingFontMetrics.descent
            bottomPadding: topPadding
            font.pixelSize: QmlUtils.fontMetrics.height
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

    contentItem: GridLayout {
        rowSpacing: Kirigami.Units.mediumSpacing
        columnSpacing: Kirigami.Units.mediumSpacing * 4
        columns: 2
        rows: 2

        Kirigami.Heading {
            Layout.column: 0; Layout.row: 0
            topPadding: -captureHeadingMetrics.descent
            bottomPadding: -captureHeadingMetrics.descent + parent.rowSpacing
            text: i18n("Capture Modes")
            level: 3
            FontMetrics {
                id: captureHeadingMetrics
            }
        }

        CaptureModeButtonsColumn {
            Layout.column: 0; Layout.row: 1
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.fillHeight: true
        }

        Kirigami.Heading {
            Layout.column: 1; Layout.row: 0
            topPadding: -captureHeadingMetrics.descent
            bottomPadding: -captureHeadingMetrics.descent + parent.rowSpacing
            text: i18n("Capture Settings")
            level: 3
        }

        CaptureSettingsColumn {
            Layout.column: 1; Layout.row: 1
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.fillHeight: true
            // Button ToolButton
            RowLayout {
                Layout.fillWidth: true
                spacing: parent.spacing
                QQC.Button {
                    icon.name: "configure"
                    text: i18n("Configure Spectacle…")
                    onClicked: contextWindow.showPreferencesDialog()
                }
                QQC.ToolButton {
                    flat: false
                    icon.name: "help-contents"
                    text: i18n("Help")
                    down: pressed || HelpMenu.visible
                    Accessible.role: Accessible.ButtonMenu
                    onPressed: HelpMenu.popup(this)
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
