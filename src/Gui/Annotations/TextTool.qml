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
        && document.selectedItem.options & AnnotationTool.TextOption
        && (document.tool.options & AnnotationTool.TextOption
            || document.tool.type === AnnotationTool.SelectTool)

    state: shouldShow ? "active" : "inactive"

    sourceComponent: T.TextField {
        id: textField
        readonly property bool mirrored: effectiveHorizontalAlignment === TextInput.AlignRight

        LayoutMirroring.enabled: false
        LayoutMirroring.childrenInherit: true

        Binding on implicitWidth {
            value: root.document.selectedItem.mousePath.boundingRect.width
            restoreMode: Binding.RestoreNone
            when: root.shouldShow
        }
        Binding on implicitHeight {
            value: root.document.selectedItem.mousePath.boundingRect.height
            restoreMode: Binding.RestoreNone
            when: root.shouldShow
        }
        Binding {
            target: root
            property: "x"
            when: root.shouldShow
            value: root.document.selectedItem.mousePath.boundingRect.x
            restoreMode: Binding.RestoreNone
        }
        Binding {
            target: root
            property: "y"
            when: root.shouldShow
            value: root.document.selectedItem.mousePath.boundingRect.y
            restoreMode: Binding.RestoreNone
        }
        property color textColor
        Binding on textColor {
            value: root.document.selectedItem.options & AnnotationTool.TextOption ?
                root.document.selectedItem.fontColor : root.document.tool.fontColor
            restoreMode: Binding.RestoreNone
            when: root.shouldShow
        }
        color: Qt.rgba(textColor.r, textColor.g, textColor.b, 0)
        Binding on font {
            value: root.document.selectedItem.options & AnnotationTool.TextOption ?
                root.document.selectedItem.font : root.document.tool.font
            restoreMode: Binding.RestoreNone
            when: root.shouldShow
        }

        focus: true
        selectByMouse: true
        selectionColor: Qt.rgba(1-textColor.r, 1-textColor.g, 1-textColor.b, 1)
        selectedTextColor: Qt.rgba(textColor.r, textColor.g, textColor.b, 1)
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
                color: Qt.rgba(textField.textColor.r, textField.textColor.g, textField.textColor.b, 1)
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
            value: root.document.selectedItem.text
            restoreMode: Binding.RestoreNone
            when: root.shouldShow
        }
        onTextChanged: {
            if (root.document.selectedItem.text === text) {
                return
            }
            let wasEmpty = root.document.selectedItem.text.length === 0
            root.document.selectedItem.text = text
            if (wasEmpty) {
                root.document.selectedItem.commitChanges()
            } else {
                commitChangesTimer.restart()
            }
        }

        Keys.onDeletePressed: (event) => {
            event.accepted = text.length === 0
            if (event.accepted) {
                root.document.deleteSelectedItem()
            }
        }

        Timer {
            id: commitChangesTimer
            interval: 250
            onTriggered: root.document.selectedItem.commitChanges()
        }

        Connections {
            target: root.document
            function onSelectedItemWrapperChanged() {
                commitChangesTimer.stop()
            }
        }

        leftInset: -background.effectiveStrokeWidth
        rightInset: -background.effectiveStrokeWidth
        topInset: -background.effectiveStrokeWidth
        bottomInset: -background.effectiveStrokeWidth
        background: SelectionBackground {
            zoom: root.viewport.effectiveZoom
        }

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
                target: null
                cursorShape: Qt.SizeAllCursor
                dragThreshold: 0
                onActiveTranslationChanged: if (active) {
                    let dx = activeTranslation.x / viewport.effectiveZoom
                    let dy = activeTranslation.y / viewport.effectiveZoom
                    root.document.selectedItem.transform(dx, dy)
                }
                onActiveChanged: if (!active) {
                    root.document.selectedItem.commitChanges()
                }
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

