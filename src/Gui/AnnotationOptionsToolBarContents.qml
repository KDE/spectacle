/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private
import "Annotations"

Row {
    id: root

    readonly property AnnotationDocument document: SpectacleCore.annotationDocument
    readonly property AnnotationTool tool: document.tool
    readonly property SelectedItemWrapper selectedItem: document.selectedItem

    readonly property bool useSelectionOptions: tool.type === AnnotationTool.SelectTool || (tool.type === AnnotationTool.TextTool && selectedItem.options & AnnotationTool.TextOption)
    readonly property int options: useSelectionOptions ? selectedItem.options : tool.options
    property int displayMode: QQC.AbstractButton.TextBesideIcon
    property int focusPolicy: Qt.StrongFocus
    readonly property bool mirrored: effectiveLayoutDirection === Qt.RightToLeft

    clip: childrenRect.width > width || childrenRect.height > height
    spacing: Kirigami.Units.mediumSpacing

    Timer {
        id: commitChangesTimer
        interval: 250
        onTriggered: root.selectedItem.commitChanges()
    }

    Connections {
        target: root.document
        function onSelectedItemWrapperChanged() {
            commitChangesTimer.stop()
        }
    }

    component ToolButton: TtToolButton {
        focusPolicy: root.focusPolicy
        display: root.displayMode
    }

    Loader { // stroke
        id: strokeLoader
        anchors.verticalCenter: parent.verticalCenter
        visible: active
        active: root.options & AnnotationTool.StrokeOption
        sourceComponent: Row {
            spacing: root.spacing

            QQC.CheckBox {
                anchors.verticalCenter: parent.verticalCenter
                text: i18n("Stroke:")
                checked: colorRect.color.a > 0
                onToggled: if (root.useSelectionOptions) {
                    root.selectedItem.strokeColor.a = checked
                } else {
                    root.tool.strokeColor.a = checked
                }
            }

            QQC.SpinBox {
                id: spinBox
                function setStrokeWidth() {
                    if (root.useSelectionOptions && root.selectedItem.strokeWidth !== spinBox.value) {
                        root.selectedItem.strokeWidth = spinBox.value
                        commitChangesTimer.restart()
                    } else {
                        root.tool.strokeWidth = spinBox.value
                    }
                }
                anchors.verticalCenter: parent.verticalCenter
                from: fillLoader.active ? 0 : 1
                to: 99
                stepSize: 1
                value: root.useSelectionOptions ? root.selectedItem.strokeWidth : root.tool.strokeWidth
                textFromValue: (value, locale) => {
                    // we don't use the locale here because the px suffix
                    // needs to be treated as a translatable string, which
                    // doesn't take into account the locale passed in here.
                    // but it's going to be the application locale
                    // which ki18n uses so it doesn't matter that much
                    // unless someone decides to set the locale for a specific
                    // part of spectacle in the future.
                    return i18ncp("px: pixels", "%1px", "%1px", Math.round(value))
                }
                valueFromText: (text, locale) => {
                    return Number.fromLocaleString(locale, text.replace(/\D/g,''))
                }
                QQC.ToolTip.text: i18n("Stroke Width")
                QQC.ToolTip.visible: hovered
                QQC.ToolTip.delay: Kirigami.Units.toolTipDelay
                // not using onValueModified because of https://bugreports.qt.io/browse/QTBUG-91281
                /* When we change the value of the control, we set the corresponding property in the
                 * annotation tool or selected annotation (if we have a selected annotation).
                 * If we have a selected annotation, we then restart a timer to commit the change as
                 * a new undoable item.
                 * We delay the call for doing the things above to prevent the property in the
                 * selected annotation from being set to the control's value before the control's
                 * value is set to the value from the selected annotation when changing selected
                 * annotations.
                 * If we don't do this, the property for a selected annotation can be unintentionally
                 * set to the value from the previous selected action.
                 * In the initial patch porting to Qt Quick, I originally used a Binding object with
                 * `delayed: true`, but that actually didn't prevent the issue. For some reason,
                 * using callLater in a signal handler did, so that's what I went with.
                 */
                onValueChanged: Qt.callLater(setStrokeWidth)
                Binding {
                    target: spinBox.contentItem
                    property: "horizontalAlignment"
                    value: Text.AlignRight
                    restoreMode: Binding.RestoreNone
                }
                TextContextMenuConnection {
                    target: spinBox.contentItem
                }
            }

            ToolButton {
                anchors.verticalCenter: parent.verticalCenter
                display: QQC.ToolButton.IconOnly
                QQC.ToolTip.text: i18n("Stroke Color")
                Rectangle { // should we use some kind of image provider instead?
                    id: colorRect
                    anchors.centerIn: parent
                    width: Kirigami.Units.gridUnit
                    height: Kirigami.Units.gridUnit
                    radius: height / 2
                    color: root.useSelectionOptions ? root.selectedItem.strokeColor : root.tool.strokeColor
                    border.color: Qt.rgba(parent.palette.windowText.r,
                                          parent.palette.windowText.g,
                                          parent.palette.windowText.b, 0.5)
                    border.width: 1
                }
                onClicked: {
                    contextWindow.showColorDialog(AnnotationTool.StrokeOption)
                }
            }
        }
    }

    QQC.ToolSeparator {
        anchors.verticalCenter: parent.verticalCenter
        visible: strokeLoader.visible && (root.options & ~AnnotationTool.StrokeOption) > AnnotationTool.StrokeOption
        height: QmlUtils.iconTextButtonHeight
    }

    Loader { // fill
        id: fillLoader
        anchors.verticalCenter: parent.verticalCenter
        visible: active
        active: root.options & AnnotationTool.FillOption
        sourceComponent: Row {
            spacing: root.spacing

            QQC.CheckBox {
                anchors.verticalCenter: parent.verticalCenter
                text: i18n("Fill:")
                checked: colorRect.color.a > 0
                onToggled: if (root.useSelectionOptions) {
                    root.selectedItem.fillColor.a = checked
                } else {
                    root.tool.fillColor.a = checked
                }
            }

            ToolButton {
                anchors.verticalCenter: parent.verticalCenter
                display: QQC.ToolButton.IconOnly
                QQC.ToolTip.text: i18n("Fill Color")
                Rectangle {
                    id: colorRect
                    anchors.centerIn: parent
                    width: Kirigami.Units.gridUnit
                    height: Kirigami.Units.gridUnit
                    radius: height / 2
                    color: root.useSelectionOptions ? root.selectedItem.fillColor : root.tool.fillColor
                    border.color: Qt.rgba(parent.palette.windowText.r,
                                          parent.palette.windowText.g,
                                          parent.palette.windowText.b, 0.5)
                    border.width: 1
                }
                onClicked: {
                    contextWindow.showColorDialog(AnnotationTool.FillOption)
                }
            }
        }
    }

    QQC.ToolSeparator {
        anchors.verticalCenter: parent.verticalCenter
        visible: fillLoader.visible && (root.options & ~AnnotationTool.FillOption) > AnnotationTool.FillOption
        height: QmlUtils.iconTextButtonHeight
    }

    Loader { // strength
        id: strengthLoader
        anchors.verticalCenter: parent.verticalCenter
        visible: active
        active: root.options & AnnotationTool.StrengthOption
        sourceComponent: Row {
            spacing: root.spacing
            leftPadding: spacing
            rightPadding: spacing

            QQC.Label {
                width: dprRound(implicitWidth)
                anchors.verticalCenter: parent.verticalCenter
                text: i18nc("@label:slider strength of effect", "Strength:")
            }

            QQC.Slider {
                id: slider
                readonly property real strength: root.useSelectionOptions ? root.selectedItem.strength : root.tool.strength
                anchors.verticalCenter: parent.verticalCenter
                function setStrength() {
                    if (root.useSelectionOptions && root.selectedItem.strength !== slider.value) {
                        root.selectedItem.strength = slider.value
                        commitChangesTimer.restart()
                    } else {
                        root.tool.strength = slider.value
                    }
                }
                height: QmlUtils.iconTextButtonHeight
                from: 0
                to: 1
                value: strength
                QQC.ToolTip.text: i18nc("@info:tooltip", "The strength of the effect.")
                QQC.ToolTip.visible: hovered
                QQC.ToolTip.delay: Kirigami.Units.toolTipDelay
                onMoved: setStrength()
            }
        }
    }

    QQC.ToolSeparator {
        anchors.verticalCenter: parent.verticalCenter
        visible: strengthLoader.visible && (root.options & ~AnnotationTool.StrengthOption) > AnnotationTool.StrengthOption
        height: QmlUtils.iconTextButtonHeight
    }

    Loader { // font
        id: fontLoader
        anchors.verticalCenter: parent.verticalCenter
        visible: active
        active: root.options & AnnotationTool.FontOption
        sourceComponent: Row {
            spacing: root.spacing

            QQC.Label {
                leftPadding: root.mirrored ? 0 : parent.spacing
                rightPadding: root.mirrored ? parent.spacing : 0
                width: contextWindow.dprRound(implicitWidth)
                anchors.verticalCenter: parent.verticalCenter
                text: i18n("Font:")
            }

            ToolButton {
                anchors.verticalCenter: parent.verticalCenter
                implicitWidth: contextWindow.dprRound(implicitContentWidth)
                display: QQC.ToolButton.TextOnly
                contentItem: QQC.Label {
                    readonly property font currentFont: root.useSelectionOptions ? root.selectedItem.font : root.tool.font
                    leftPadding: Kirigami.Units.mediumSpacing
                    rightPadding: leftPadding
                    font.family: currentFont.family
                    font.styleName: currentFont.styleName
                    text: font.styleName !== "" ?
                        i18ncp("%2 font family, %3 font style name, %1 font point size", "%2 %3 %1pt", "%2 %3 %1pts", currentFont.pointSize, font.family, font.styleName) :
                        i18ncp("%2 font family %1 font point size", "%2 %1pt", "%2 %1pts", currentFont.pointSize, font.family)
                    elide: Text.ElideNone
                    wrapMode: Text.NoWrap
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: contextWindow.showFontDialog()
            }

            ToolButton {
                anchors.verticalCenter: parent.verticalCenter
                display: QQC.ToolButton.IconOnly
                QQC.ToolTip.text: i18n("Font Color")
                Rectangle {
                    id: colorRect
                    anchors.centerIn: parent
                    width: Kirigami.Units.gridUnit
                    height: Kirigami.Units.gridUnit
                    radius: height / 2
                    color: root.useSelectionOptions ? root.selectedItem.fontColor : root.tool.fontColor
                    border.color: Qt.rgba(parent.palette.windowText.r,
                                          parent.palette.windowText.g,
                                          parent.palette.windowText.b, 0.5)
                    border.width: 1
                }
                onClicked: {
                    contextWindow.showColorDialog(AnnotationTool.FontOption)
                }
            }
        }
    }

    QQC.ToolSeparator {
        anchors.verticalCenter: parent.verticalCenter
        visible: fontLoader.visible && (root.options & ~AnnotationTool.FontOption) > AnnotationTool.FontOption
        height: QmlUtils.iconTextButtonHeight
    }

    Loader { // stroke
        id: numberLoader
        anchors.verticalCenter: parent.verticalCenter
        visible: active
        active: root.options & AnnotationTool.NumberOption
        sourceComponent: Row {
            spacing: root.spacing

            QQC.Label {
                leftPadding: root.mirrored ? 0 : parent.spacing
                rightPadding: root.mirrored ? parent.spacing : 0
                width: contextWindow.dprRound(implicitWidth)
                anchors.verticalCenter: parent.verticalCenter
                text: i18n("Number:")
            }

            QQC.SpinBox {
                id: spinBox
                readonly property int number: root.useSelectionOptions ? root.selectedItem.number : root.tool.number
                anchors.verticalCenter: parent.verticalCenter
                function setNumber() {
                    if (root.useSelectionOptions && root.selectedItem.number !== spinBox.value) {
                        root.selectedItem.number = spinBox.value
                        commitChangesTimer.restart()
                    } else {
                        root.tool.number = spinBox.value
                    }
                }
                from: -99
                to: Math.max(999, number + 1)
                stepSize: 1
                value: number
                QQC.ToolTip.text: i18n("Number for number annotations")
                QQC.ToolTip.visible: hovered
                QQC.ToolTip.delay: Kirigami.Units.toolTipDelay
                // not using onValueModified because of https://bugreports.qt.io/browse/QTBUG-91281
                onValueChanged: Qt.callLater(setNumber)
                Binding {
                    target: spinBox.contentItem
                    property: "horizontalAlignment"
                    value: Text.AlignRight
                    restoreMode: Binding.RestoreNone
                }
                TextContextMenuConnection {
                    target: spinBox.contentItem
                }
            }
        }
    }

    QQC.ToolSeparator {
        anchors.verticalCenter: parent.verticalCenter
        visible: numberLoader.visible && (root.options & ~AnnotationTool.NumberOption) > AnnotationTool.NumberOption
        height: QmlUtils.iconTextButtonHeight
    }

    QQC.CheckBox {
        id: shadowCheckBox
        anchors.verticalCenter: parent.verticalCenter
        visible: root.options & AnnotationTool.ShadowOption
        text: i18n("Shadow")
        checked: {
            if (root.tool.type === AnnotationTool.TextTool && root.selectedItem.options & AnnotationTool.TextOption) {
                return root.tool.shadow;
            } else if (root.useSelectionOptions) {
                return root.selectedItem.shadow;
            } else {
                return root.tool.shadow;
            }
        }
        onToggled: {
            let changed = false
            if (root.tool.type === AnnotationTool.TextTool && root.selectedItem.options & AnnotationTool.TextOption) {
                changed = root.selectedItem.shadow !== checked
                root.selectedItem.shadow = checked;
                root.tool.shadow = checked;
            } else if (root.useSelectionOptions) {
                changed = root.selectedItem.shadow !== checked
                root.selectedItem.shadow = checked;
            } else {
                root.tool.shadow = checked;
            }
            if (changed) {
                commitChangesTimer.restart();
            }
        }
    }
}
