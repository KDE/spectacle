/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQml 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Templates 2.15 as T
import org.kde.kirigami 2.19 as Kirigami
import org.kde.spectacle.private 1.0
import "Annotations"

ButtonGrid {
    id: root
    readonly property int activeSectionCount: strokeLoader.active + fillLoader.active + fontLoader.active
    readonly property bool isSelectedActionOptions: AnnotationDocument.tool.type === AnnotationDocument.ChangeAction || (AnnotationDocument.tool.type === AnnotationDocument.Text && AnnotationDocument.selectedAction.type === AnnotationDocument.Text)

    animationsEnabled: false

    Timer {
        id: commitChangesTimer
        interval: 250
        onTriggered: AnnotationDocument.selectedAction.commitChanges()
    }

    Connections {
        target: AnnotationDocument
        function onSelectedActionWrapperChanged() {
            commitChangesTimer.stop()
        }
    }

    component ToolButton: QQC2.ToolButton {
        implicitHeight: QmlUtils.iconTextButtonHeight
        width: display === QQC2.ToolButton.IconOnly ? height : implicitWidth
        focusPolicy: root.focusPolicy
        display: root.displayMode
        QQC2.ToolTip.text: text
        QQC2.ToolTip.visible: (hovered || pressed) && display === QQC2.ToolButton.IconOnly
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
    }

    Loader { // stroke
        id: strokeLoader
        visible: active
        active: isSelectedActionOptions ?
            AnnotationDocument.selectedAction.options & AnnotationTool.Stroke
            : AnnotationDocument.tool.options & AnnotationTool.Stroke
        sourceComponent: Row {
            spacing: root.spacing

            QQC2.CheckBox {
                anchors.verticalCenter: parent.verticalCenter
                text: i18n("Stroke:")
                checked: colorRect.color.a > 0
                onToggled: if (root.isSelectedActionOptions) {
                    AnnotationDocument.selectedAction.strokeColor.a = checked
                } else {
                    AnnotationDocument.tool.strokeColor.a = checked
                }
            }

            QQC2.SpinBox {
                id: spinBox
                function setStrokeWidth() {
                    if (root.isSelectedActionOptions) {
                        AnnotationDocument.selectedAction.strokeWidth = spinBox.value
                        commitChangesTimer.restart()
                    } else {
                        AnnotationDocument.tool.strokeWidth = spinBox.value
                    }
                }
                anchors.verticalCenter: parent.verticalCenter
                from: fillLoader.active ? 0 : 1
                to: 99
                stepSize: 1
                value: root.isSelectedActionOptions ?
                    AnnotationDocument.selectedAction.strokeWidth
                    : AnnotationDocument.tool.strokeWidth
                textFromValue: (value, locale) => {
                    return Number(Math.round(value)).toLocaleString(locale, 'f', 0) + "px"
                }
                valueFromText: (text, locale) => {
                    return Number.fromLocaleString(locale, text.replace(/\D/g,''))
                }
                QQC2.ToolTip.text: i18n("Stroke Width")
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                // not using onValueModified because of https://bugreports.qt.io/browse/QTBUG-91281
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
                display: QQC2.ToolButton.IconOnly
                QQC2.ToolTip.text: i18n("Stroke Color")
                Rectangle { // should we use some kind of image provider instead?
                    id: colorRect
                    anchors.centerIn: parent
                    width: Kirigami.Units.gridUnit
                    height: Kirigami.Units.gridUnit
                    radius: height / 2
                    color: root.isSelectedActionOptions ?
                        AnnotationDocument.selectedAction.strokeColor
                        : AnnotationDocument.tool.strokeColor
                    border.color: Qt.rgba(parent.palette.windowText.r,
                                          parent.palette.windowText.g,
                                          parent.palette.windowText.b, 0.5)
                    border.width: 1
                }
                onClicked: {
                    contextWindow.showColorDialog(AnnotationTool.Stroke)
                }
            }
        }
    }

    QQC2.ToolSeparator {
        visible: strokeLoader.visible && fillLoader.visible
        height: QmlUtils.iconTextButtonHeight
    }

    Loader { // fill
        id: fillLoader
        visible: active
        active: isSelectedActionOptions ?
            AnnotationDocument.selectedAction.options & AnnotationTool.Fill
            : AnnotationDocument.tool.options & AnnotationTool.Fill
        sourceComponent: Row {
            spacing: root.spacing

            QQC2.CheckBox {
                anchors.verticalCenter: parent.verticalCenter
                text: i18n("Fill:")
                checked: colorRect.color.a > 0
                onToggled: if (root.isSelectedActionOptions) {
                    AnnotationDocument.selectedAction.fillColor.a = checked
                } else {
                    AnnotationDocument.tool.fillColor.a = checked
                }
            }

            ToolButton {
                anchors.verticalCenter: parent.verticalCenter
                display: QQC2.ToolButton.IconOnly
                QQC2.ToolTip.text: i18n("Fill Color")
                Rectangle {
                    id: colorRect
                    anchors.centerIn: parent
                    width: Kirigami.Units.gridUnit
                    height: Kirigami.Units.gridUnit
                    radius: height / 2
                    color: root.isSelectedActionOptions ?
                        AnnotationDocument.selectedAction.fillColor
                        : AnnotationDocument.tool.fillColor
                    border.color: Qt.rgba(parent.palette.windowText.r,
                                          parent.palette.windowText.g,
                                          parent.palette.windowText.b, 0.5)
                    border.width: 1
                }
                onClicked: {
                    contextWindow.showColorDialog(AnnotationTool.Fill)
                }
            }
        }
    }

    QQC2.ToolSeparator {
        visible: fillLoader.visible && fontLoader.visible
        height: QmlUtils.iconTextButtonHeight
    }

    Loader { // font
        id: fontLoader
        visible: active
        active: isSelectedActionOptions ?
            AnnotationDocument.selectedAction.options & AnnotationTool.Font
            : AnnotationDocument.tool.options & AnnotationTool.Font
        sourceComponent: Row {
            spacing: root.spacing

            QQC2.Label {
                leftPadding: root.mirrored ? 0 : parent.spacing
                rightPadding: root.mirrored ? parent.spacing : 0
                width: contextWindow.dprRound(implicitWidth)
                anchors.verticalCenter: parent.verticalCenter
                text: i18n("Font:")
            }

            ToolButton {
                implicitWidth: contextWindow.dprRound(implicitContentWidth)
                display: QQC2.ToolButton.TextOnly
                contentItem: QQC2.Label {
                    readonly property font currentFont: root.isSelectedActionOptions ?
                        AnnotationDocument.selectedAction.font
                        : AnnotationDocument.tool.font
                    leftPadding: Kirigami.Units.mediumSpacing
                    rightPadding: leftPadding
                    font.family: currentFont.family
                    font.styleName: currentFont.styleName
                    text: font.family
                        + (font.styleName ? ` ${font.styleName}` : "")
                        + ` ${currentFont.pointSize}pts`
                    elide: Text.ElideNone
                    wrapMode: Text.NoWrap
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: contextWindow.showFontDialog()
            }

            ToolButton {
                anchors.verticalCenter: parent.verticalCenter
                display: QQC2.ToolButton.IconOnly
                QQC2.ToolTip.text: i18n("Font Color")
                Rectangle {
                    id: colorRect
                    anchors.centerIn: parent
                    width: Kirigami.Units.gridUnit
                    height: Kirigami.Units.gridUnit
                    radius: height / 2
                    color: root.isSelectedActionOptions ?
                        AnnotationDocument.selectedAction.fontColor
                        : AnnotationDocument.tool.fontColor
                    border.color: Qt.rgba(parent.palette.windowText.r,
                                          parent.palette.windowText.g,
                                          parent.palette.windowText.b, 0.5)
                    border.width: 1
                }
                onClicked: {
                    contextWindow.showColorDialog(AnnotationTool.Font)
                }
            }
        }
    }

    QQC2.ToolSeparator {
        visible: fontLoader.visible && numberLoader.visible
        height: QmlUtils.iconTextButtonHeight
    }

    Loader { // stroke
        id: numberLoader
        visible: active
        active: isSelectedActionOptions ?
            AnnotationDocument.selectedAction.type === AnnotationDocument.Number
            : AnnotationDocument.tool.type === AnnotationDocument.Number
        sourceComponent: Row {
            spacing: root.spacing

            QQC2.Label {
                leftPadding: root.mirrored ? 0 : parent.spacing
                rightPadding: root.mirrored ? parent.spacing : 0
                width: contextWindow.dprRound(implicitWidth)
                anchors.verticalCenter: parent.verticalCenter
                text: i18n("Number:")
            }

            QQC2.SpinBox {
                id: spinBox
                readonly property int number: root.isSelectedActionOptions ?
                    AnnotationDocument.selectedAction.number : AnnotationDocument.tool.number
                anchors.verticalCenter: parent.verticalCenter
                function setNumber() {
                    if (root.isSelectedActionOptions) {
                        AnnotationDocument.selectedAction.number = spinBox.value
                        commitChangesTimer.restart()
                    } else {
                        AnnotationDocument.tool.number = spinBox.value
                    }
                }
                from: -99
                to: Math.max(999, number + 1)
                stepSize: 1
                value: number
                QQC2.ToolTip.text: i18n("Number for number annotations")
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
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

    QQC2.ToolSeparator {
        visible: shadowCheckBox.visible
        height: QmlUtils.iconTextButtonHeight
    }

    QQC2.CheckBox {
        id: shadowCheckBox
        visible: root.isSelectedActionOptions ?
            AnnotationDocument.selectedAction.options & AnnotationTool.Shadow
            : AnnotationDocument.tool.options & AnnotationTool.Shadow
        text: i18n("Shadow")
        checked: {
            if (AnnotationDocument.tool.type === AnnotationDocument.Text && AnnotationDocument.selectedAction.type === AnnotationDocument.Text) {
                return AnnotationDocument.tool.shadow;
            } else if (root.isSelectedActionOptions) {
                return AnnotationDocument.selectedAction.shadow;
            } else {
                return AnnotationDocument.tool.shadow;
            }
        }
        onToggled: {
            if (AnnotationDocument.tool.type === AnnotationDocument.Text && AnnotationDocument.selectedAction.type === AnnotationDocument.Text) {
                AnnotationDocument.selectedAction.shadow = checked;
                AnnotationDocument.tool.shadow = checked;
            } else if (root.isSelectedActionOptions) {
                AnnotationDocument.selectedAction.shadow = checked;
                commitChangesTimer.restart();
            } else {
                AnnotationDocument.tool.shadow = checked;
            }
            commitChangesTimer.restart();
        }
    }
}
