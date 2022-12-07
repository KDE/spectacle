/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQml 2.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami
import org.kde.spectacle.private 1.0

Grid {
    id: root
    property int displayMode: QQC2.AbstractButton.TextBesideIcon
    property int focusPolicy: Qt.StrongFocus
    readonly property real fullButtonHeight: iconTextButtonMetrics.implicitHeight
    readonly property bool mirrored: effectiveLayoutDirection === Qt.RightToLeft
    property bool animationsEnabled: true

    function getButtonWidth(displayMode, text, icon, arrow) {
        if (displayMode === QQC2.AbstractButton.IconOnly || (!text && icon)) {
            return iconOnlyButtonMetrics.implicitWidth
        }

        let w = 0
        if (displayMode === QQC2.AbstractButton.TextOnly || (text && !icon)) {
            w += textOnlyButtonMetrics.noTextWidth + fontMetrics.boundingRect(text).width
        } else if (displayMode === QQC2.AbstractButton.TextBesideIcon && text && icon) {
            w += iconTextButtonMetrics.noTextWidth + fontMetrics.boundingRect(text).width
        } else {
            w += textOnlyButtonMetrics.noTextWidth
        }
        // NOTE: only qqc2-desktop-style and qqc2-breeze-style have showMenuArrow
        if (arrow) {
            w += arrowButtonMetrics.arrowWidth
        }
        return w
    }

    clip: childrenRect.width > width || childrenRect.height > height
    horizontalItemAlignment: Grid.AlignHCenter
    verticalItemAlignment: Grid.AlignVCenter
    spacing: Kirigami.Units.mediumSpacing
    columns: flow === Grid.LeftToRight ? visibleChildren.length : 1
    rows: flow === Grid.TopToBottom ? visibleChildren.length : 1
    move: Transition {
        enabled: root.animationsEnabled
        NumberAnimation { properties: "x,y"; duration: Kirigami.Units.longDuration; easing.type: Easing.OutCubic }
    }
    add: Transition {
        enabled: root.animationsEnabled
        NumberAnimation { properties: "x,y"; duration: Kirigami.Units.longDuration; easing.type: Easing.OutCubic }
    }

    FontMetrics {
        id: fontMetrics
    }

    QQC2.ToolButton {
        id: arrowButtonMetrics
        readonly property real arrowWidth: implicitWidth - fontMetrics.boundingRect(text).width - textOnlyButtonMetrics.noTextWidth
        parent: null
        display: QQC2.AbstractButton.TextOnly
        text: "text metrics"
        visible: false
        enabled: false
        // NOTE: only qqc2-desktop-style and qqc2-breeze-style have showMenuArrow
        Component.onCompleted: if (background.hasOwnProperty("showMenuArrow")) {
            background.showMenuArrow = true
        }
    }

    QQC2.ToolButton {
        id: iconTextButtonMetrics
        readonly property real noTextWidth: implicitWidth - fontMetrics.boundingRect(text).width
        parent: null
        display: QQC2.AbstractButton.TextBesideIcon
        icon.name: "edit-copy"
        text: "text metrics"
        visible: false
        enabled: false
    }

    QQC2.ToolButton {
        id: textOnlyButtonMetrics
        readonly property real noTextWidth: implicitWidth - fontMetrics.boundingRect(text).width
        parent: null
        display: QQC2.AbstractButton.TextOnly
        text: "text metrics"
        visible: false
        enabled: false
    }

    QQC2.ToolButton {
        id: iconOnlyButtonMetrics
        parent: null
        display: QQC2.AbstractButton.IconOnly
        icon.name: "edit-copy"
        visible: false
        enabled: false
    }
}
