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
    readonly property bool useSelectionOptions: AnnotationDocument.tool.type === AnnotationTool.SelectTool || (AnnotationDocument.tool.type === AnnotationTool.TextTool && AnnotationDocument.selectedItem.options & AnnotationTool.TextOption)
    property int displayMode: QQC.AbstractButton.TextBesideIcon
    property int focusPolicy: Qt.StrongFocus
    readonly property bool mirrored: effectiveLayoutDirection === Qt.RightToLeft

    clip: childrenRect.width > width || childrenRect.height > height
    spacing: Kirigami.Units.mediumSpacing

    Timer {
        id: commitChangesTimer
        interval: 250
        onTriggered: AnnotationDocument.selectedItem.commitChanges()
    }

    Connections {
        target: AnnotationDocument
        function onSelectedItemWrapperChanged() {
            commitChangesTimer.stop()
        }
    }

    component ToolButton: QQC.ToolButton {
        implicitHeight: QmlUtils.iconTextButtonHeight
        width: display === QQC.ToolButton.IconOnly ? height : implicitWidth
        focusPolicy: root.focusPolicy
        display: root.displayMode
        QQC.ToolTip.text: text
        QQC.ToolTip.visible: (hovered || pressed) && display === QQC.ToolButton.IconOnly
        QQC.ToolTip.delay: Kirigami.Units.toolTipDelay
    }

    Loader { // stroke
        id: strokeLoader
        anchors.verticalCenter: parent.verticalCenter
        visible: active
        active: useSelectionOptions ?
            AnnotationDocument.selectedItem.options & AnnotationTool.StrokeOption
            : AnnotationDocument.tool.options & AnnotationTool.StrokeOption
        sourceComponent: Row {
            spacing: root.spacing

            QQC.CheckBox {
                anchors.verticalCenter: parent.verticalCenter
                text: i18n("Stroke:")
                checked: colorRect.color.a > 0
                onToggled: if (root.useSelectionOptions) {
                    AnnotationDocument.selectedItem.strokeColor.a = checked
                } else {
                    AnnotationDocument.tool.strokeColor.a = checked
                }
            }

            QQC.SpinBox {
                id: spinBox
                function setStrokeWidth() {
                    if (root.useSelectionOptions
                        && AnnotationDocument.selectedItem.strokeWidth !== spinBox.value) {
                        AnnotationDocument.selectedItem.strokeWidth = spinBox.value
                        commitChangesTimer.restart()
                    } else {
                        AnnotationDocument.tool.strokeWidth = spinBox.value
                    }
                }
                anchors.verticalCenter: parent.verticalCenter
                from: fillLoader.active ? 0 : 1
                to: 99
                stepSize: 1
                value: root.useSelectionOptions ?
                    AnnotationDocument.selectedItem.strokeWidth
                    : AnnotationDocument.tool.strokeWidth
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
                    color: root.useSelectionOptions ?
                        AnnotationDocument.selectedItem.strokeColor
                        : AnnotationDocument.tool.strokeColor
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
        visible: strokeLoader.visible && fillLoader.visible
        height: QmlUtils.iconTextButtonHeight
    }

    Loader { // fill
        id: fillLoader
        anchors.verticalCenter: parent.verticalCenter
        visible: active
        active: useSelectionOptions ?
            AnnotationDocument.selectedItem.options & AnnotationTool.FillOption
            : AnnotationDocument.tool.options & AnnotationTool.FillOption
        sourceComponent: Row {
            spacing: root.spacing

            QQC.CheckBox {
                anchors.verticalCenter: parent.verticalCenter
                text: i18n("Fill:")
                checked: colorRect.color.a > 0
                onToggled: if (root.useSelectionOptions) {
                    AnnotationDocument.selectedItem.fillColor.a = checked
                } else {
                    AnnotationDocument.tool.fillColor.a = checked
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
                    color: root.useSelectionOptions ?
                        AnnotationDocument.selectedItem.fillColor
                        : AnnotationDocument.tool.fillColor
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
        visible: fillLoader.visible && fontLoader.visible
        height: QmlUtils.iconTextButtonHeight
    }

    Loader { // font
        id: fontLoader
        anchors.verticalCenter: parent.verticalCenter
        visible: active
        active: useSelectionOptions ?
            AnnotationDocument.selectedItem.options & AnnotationTool.FontOption
            : AnnotationDocument.tool.options & AnnotationTool.FontOption
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
                    readonly property font currentFont: root.useSelectionOptions ?
                        AnnotationDocument.selectedItem.font
                        : AnnotationDocument.tool.font
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
                    color: root.useSelectionOptions ?
                        AnnotationDocument.selectedItem.fontColor
                        : AnnotationDocument.tool.fontColor
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
        visible: fontLoader.visible && numberLoader.visible
        height: QmlUtils.iconTextButtonHeight
    }

    Loader { // stroke
        id: numberLoader
        anchors.verticalCenter: parent.verticalCenter
        visible: active
        active: useSelectionOptions ?
            AnnotationDocument.selectedItem.options & AnnotationTool.NumberOption
            : AnnotationDocument.tool.options & AnnotationTool.NumberOption
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
                readonly property int number: root.useSelectionOptions ?
                    AnnotationDocument.selectedItem.number : AnnotationDocument.tool.number
                anchors.verticalCenter: parent.verticalCenter
                function setNumber() {
                    if (root.useSelectionOptions
                        && AnnotationDocument.selectedItem.number !== spinBox.value) {
                        AnnotationDocument.selectedItem.number = spinBox.value
                        commitChangesTimer.restart()
                    } else {
                        AnnotationDocument.tool.number = spinBox.value
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
            }
        }
    }

    QQC.ToolSeparator {
        anchors.verticalCenter: parent.verticalCenter
        visible: shadowCheckBox.visible
        height: QmlUtils.iconTextButtonHeight
    }

    QQC.CheckBox {
        id: shadowCheckBox
        anchors.verticalCenter: parent.verticalCenter
        visible: root.useSelectionOptions ?
            AnnotationDocument.selectedItem.options & AnnotationTool.ShadowOption
            : AnnotationDocument.tool.options & AnnotationTool.ShadowOption
        text: i18n("Shadow")
        checked: {
            if (AnnotationDocument.tool.type === AnnotationTool.TextTool && AnnotationDocument.selectedItem.options & AnnotationTool.TextOption) {
                return AnnotationDocument.tool.shadow;
            } else if (root.useSelectionOptions) {
                return AnnotationDocument.selectedItem.shadow;
            } else {
                return AnnotationDocument.tool.shadow;
            }
        }
        onToggled: {
            let changed = false
            if (AnnotationDocument.tool.type === AnnotationTool.TextTool && AnnotationDocument.selectedItem.options & AnnotationTool.TextOption) {
                changed = AnnotationDocument.selectedItem.shadow !== checked
                AnnotationDocument.selectedItem.shadow = checked;
                AnnotationDocument.tool.shadow = checked;
            } else if (root.useSelectionOptions) {
                changed = AnnotationDocument.selectedItem.shadow !== checked
                AnnotationDocument.selectedItem.shadow = checked;
            } else {
                AnnotationDocument.tool.shadow = checked;
            }
            if (changed) {
                commitChangesTimer.restart();
            }
        }
    }
}
