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
    readonly property bool isSelectedActionOptions: AnnotationDocument.tool.type === AnnotationDocument.ChangeAction || (AnnotationDocument.tool.type === AnnotationDocument.Text && AnnotationDocument.selectedAction.type === AnnotationDocument.Text)
    property int displayMode: QQC.AbstractButton.TextBesideIcon
    property int focusPolicy: Qt.StrongFocus
    readonly property bool mirrored: effectiveLayoutDirection === Qt.RightToLeft

    clip: childrenRect.width > width || childrenRect.height > height
    spacing: Kirigami.Units.mediumSpacing

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
        active: isSelectedActionOptions ?
            AnnotationDocument.selectedAction.options & AnnotationTool.Stroke
            : AnnotationDocument.tool.options & AnnotationTool.Stroke
        sourceComponent: Row {
            spacing: root.spacing

            QQC.CheckBox {
                anchors.verticalCenter: parent.verticalCenter
                text: i18n("Stroke:")
                checked: colorRect.color.a > 0
                onToggled: if (root.isSelectedActionOptions) {
                    AnnotationDocument.selectedAction.strokeColor.a = checked
                } else {
                    AnnotationDocument.tool.strokeColor.a = checked
                }
            }

            QQC.SpinBox {
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
                QQC.ToolTip.text: i18n("Stroke Width")
                QQC.ToolTip.visible: hovered
                QQC.ToolTip.delay: Kirigami.Units.toolTipDelay
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
                display: QQC.ToolButton.IconOnly
                QQC.ToolTip.text: i18n("Stroke Color")
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

    QQC.ToolSeparator {
        anchors.verticalCenter: parent.verticalCenter
        visible: strokeLoader.visible && fillLoader.visible
        height: QmlUtils.iconTextButtonHeight
    }

    Loader { // fill
        id: fillLoader
        anchors.verticalCenter: parent.verticalCenter
        visible: active
        active: isSelectedActionOptions ?
            AnnotationDocument.selectedAction.options & AnnotationTool.Fill
            : AnnotationDocument.tool.options & AnnotationTool.Fill
        sourceComponent: Row {
            spacing: root.spacing

            QQC.CheckBox {
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
                display: QQC.ToolButton.IconOnly
                QQC.ToolTip.text: i18n("Fill Color")
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

    QQC.ToolSeparator {
        anchors.verticalCenter: parent.verticalCenter
        visible: fillLoader.visible && fontLoader.visible
        height: QmlUtils.iconTextButtonHeight
    }

    Loader { // font
        id: fontLoader
        anchors.verticalCenter: parent.verticalCenter
        visible: active
        active: isSelectedActionOptions ?
            AnnotationDocument.selectedAction.options & AnnotationTool.Font
            : AnnotationDocument.tool.options & AnnotationTool.Font
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
                    readonly property font currentFont: root.isSelectedActionOptions ?
                        AnnotationDocument.selectedAction.font
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

    QQC.ToolSeparator {
        anchors.verticalCenter: parent.verticalCenter
        visible: fontLoader.visible && numberLoader.visible
        height: QmlUtils.iconTextButtonHeight
    }

    Loader { // stroke
        id: numberLoader
        anchors.verticalCenter: parent.verticalCenter
        visible: active
        active: isSelectedActionOptions ?
            AnnotationDocument.selectedAction.type === AnnotationDocument.Number
            : AnnotationDocument.tool.type === AnnotationDocument.Number
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
