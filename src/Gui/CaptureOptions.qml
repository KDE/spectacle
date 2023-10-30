/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

Column {
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
        // If recording is supported, there would be a tab bar right above this
        // label with largely the same text, creating redundancy.
        visible: !VideoPlatform.supportedRecordingModes
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
