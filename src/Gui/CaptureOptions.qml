/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQml 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami
import org.kde.spectacle.private 1.0

QQC2.Pane {
    leftPadding: Kirigami.Units.mediumSpacing * 2
        + (!mirrored ? sideBarSeparator.implicitWidth : 0)
    rightPadding: Kirigami.Units.mediumSpacing * 2
        + (mirrored ? sideBarSeparator.implicitWidth : 0)
    topPadding: Kirigami.Units.mediumSpacing * 2
    bottomPadding: Kirigami.Units.mediumSpacing * 2

    contentItem: Column {
        spacing: Kirigami.Units.mediumSpacing
        Kirigami.Heading {
            anchors.left: parent.left
            width: Math.max(implicitWidth, parent.width)
            topPadding: -captureHeadingMetrics.descent
            bottomPadding: -captureHeadingMetrics.descent + parent.spacing
            text: i18n("Take a new screenshot")
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            level: 3
            FontMetrics {
                id: captureHeadingMetrics
            }
        }
        CaptureModeButtonsColumn {
            anchors.left: parent.left
            width: Math.max(implicitWidth, parent.width)
        }
        Kirigami.Heading {
            anchors.left: parent.left
            width: Math.max(implicitWidth, parent.width)
            topPadding: -captureHeadingMetrics.descent + parent.spacing
            bottomPadding: -captureHeadingMetrics.descent + parent.spacing
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            text: i18n("Capture Settings")
            level: 3
        }
        CaptureSettingsColumn {
            anchors.left: parent.left
            width: Math.max(Layout.minimumWidth, parent.width)
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
