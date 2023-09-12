/* SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private
import ".."

AnimatedLoader {
    id: root
    required property AnnotationViewport viewport
    readonly property AnnotationDocument document: viewport.document
    readonly property bool shouldShow: enabled
        && document.selectedAction.type === AnnotationDocument.Text
        && (document.tool.type === AnnotationDocument.Text
            || document.tool.type === AnnotationDocument.ChangeAction)

    state: shouldShow ? "active" : "inactive"

    sourceComponent: T.TextField {
        id: textField
        readonly property bool mirrored: effectiveHorizontalAlignment === TextInput.AlignRight

        LayoutMirroring.enabled: false
        LayoutMirroring.childrenInherit: true

        Binding on implicitWidth {
            value: Math.ceil(Math.max(contentWidth, fontMetrics.xHeight))
            restoreMode: Binding.RestoreNone
            when: root.shouldShow
        }
        Binding on implicitHeight {
            value: textField.contentHeight
            restoreMode: Binding.RestoreNone
            when: root.shouldShow
        }
        Binding on color {
            value: root.document.selectedAction.type === AnnotationDocument.Text ?
                root.document.selectedAction.fontColor : root.document.tool.fontColor
            restoreMode: Binding.RestoreNone
            when: root.shouldShow
        }
        Binding on font {
            value: root.document.selectedAction.type === AnnotationDocument.Text ?
                root.document.selectedAction.font : root.document.tool.font
            restoreMode: Binding.RestoreNone
            when: root.shouldShow
        }

        focus: true
        selectByMouse: true
        selectionColor: Qt.rgba(1-color.r, 1-color.g, 1-color.b, 1)
        selectedTextColor: Qt.rgba(color.r, color.g, color.b, 1)
        cursorPosition: {
            const mapped = mapFromItem(root.viewport, root.viewport.pressPosition)
            return positionAt(mapped.x, mapped.y)
        }
        cursorDelegate: Item {
            id: cursor
            visible: textField.cursorVisible
            Rectangle {
                // prevent the cursor from overlapping with the background
                x: textField.cursorPosition === textField.length && textField.length > 0 ?
                    -width : 0
                width: Math.max(1 / root.viewport.zoom,
                                contextWindow.dprRound(fontMetrics.xHeight / 12))
                height: parent.height
                color: Qt.rgba(textField.color.r, textField.color.g, textField.color.b, 1)
            }
            Connections {
                target: textField
                function onCursorPositionChanged() {
                    if (textField.cursorVisible) {
                        Qt.callLater(blinkAnimation.restart)
                    }
                }
            }
            SequentialAnimation {
                id: blinkAnimation
                running: textField.cursorVisible
                loops: Animation.Infinite
                PropertyAction {
                    target: cursor
                    property: "visible"
                    value: true
                }
                PauseAnimation {
                    duration: Qt.styleHints.cursorFlashTime / 2
                }
                PropertyAction {
                    target: cursor
                    property: "visible"
                    value: false
                }
                PauseAnimation {
                    duration: Qt.styleHints.cursorFlashTime / 2
                }
            }
        }

        Binding on text {
            value: root.document.selectedAction.text
            restoreMode: Binding.RestoreNone
            when: root.shouldShow
        }
        onTextEdited: {
            root.document.selectedAction.text = text
            commitChangesTimer.restart()
        }

        Timer {
            id: commitChangesTimer
            interval: 250
            onTriggered: root.document.selectedAction.commitChanges()
        }

        Connections {
            target: root.document
            function onSelectedActionWrapperChanged() {
                commitChangesTimer.stop()
            }
        }

        // these have to be set here to avoid null selectedAction errors
        Binding {
            target: root
            property: "x"
            when: root.shouldShow && !dragHandler.active
            value: root.document.selectedAction.visualGeometry.x
            restoreMode: Binding.RestoreNone
        }
        Binding {
            target: root
            property: "y"
            when: root.shouldShow && !dragHandler.active
            value: root.document.selectedAction.visualGeometry.y
            restoreMode: Binding.RestoreNone
        }
        Binding {
            target: root.document.selectedAction
            property: "visualGeometry.x"
            value: root.x
            when: root.shouldShow && dragHandler.active
            restoreMode: Binding.RestoreNone
        }
        Binding {
            target: root.document.selectedAction
            property: "visualGeometry.y"
            value: root.y
            when: root.shouldShow && dragHandler.active
            restoreMode: Binding.RestoreNone
        }

        leftInset: -background.strokeWidth
        rightInset: -background.strokeWidth
        topInset: -background.strokeWidth
        bottomInset: -background.strokeWidth
        background: SelectionBackground { zoom: root.viewport.zoom }

        FontMetrics {
            id: fontMetrics
            font: textField.font
        }

        Rectangle {
            id: handle
            implicitHeight: fontMetrics.height % 2 ? fontMetrics.height + 1 : fontMetrics.height
            implicitWidth: implicitHeight
            anchors.left: textField.effectiveHorizontalAlignment === TextInput.AlignRight ?
                parent.right : undefined
            anchors.right: textField.effectiveHorizontalAlignment === TextInput.AlignLeft ?
                parent.left : undefined
            anchors.margins: Kirigami.Units.mediumSpacing
            radius: height / 2
            color: Kirigami.Theme.backgroundColor
            Kirigami.Icon {
                height: Kirigami.Units.iconSizes.roundedIconSize(parent.height)
                width: height
                anchors.centerIn: parent
                source: "transform-move"
            }
            DragHandler {
                id: dragHandler
                target: root
                cursorShape: Qt.SizeAllCursor
                dragThreshold: 0
            }
            TapHandler {
                cursorShape: Qt.SizeAllCursor
            }
            HoverHandler {
                cursorShape: Qt.SizeAllCursor
            }
        }
        Component.onCompleted: forceActiveFocus()
    }
}

