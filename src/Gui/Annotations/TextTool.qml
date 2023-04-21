/* SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Shapes
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

AnimatedLoader {
    id: root
    required property AnnotationViewport viewport
    readonly property AnnotationDocument document: viewport.document
    readonly property bool shouldShow: enabled
        && document.selectedItem.options & AnnotationTool.TextOption
        && (document.tool.options & AnnotationTool.TextOption
            || document.tool.type === AnnotationTool.SelectTool)

    state: shouldShow ? "active" : "inactive"

    sourceComponent: T.TextArea {
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
            value: root.document.selectedItem.mousePath.boundingRect.x - root.document.canvasRect.x
            restoreMode: Binding.RestoreNone
        }
        Binding {
            target: root
            property: "y"
            when: root.shouldShow
            value: root.document.selectedItem.mousePath.boundingRect.y - root.document.canvasRect.y
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
                x: {
                    const atStart = textField.cursorPosition === 0;
                    const atEnd = textField.cursorPosition === textField.length;
                    const atNewLine = !atEnd && textField.text.charAt(textField.cursorPosition) === '\n';
                    const afterNewLine = !atStart && textField.text.charAt(textField.cursorPosition - 1) === '\n';
                    if (!atStart && (atEnd || atNewLine) && !afterNewLine) {
                        return -width;
                    }
                    return 0;
                }
                // Ensure cursor is at least 1 physical pixel thick
                width: QmlUtils.clampPx(dprRound(Math.max(1, fontMetrics.xHeight / 12)),
                                        1 / Screen.devicePixelRatio / root.viewport.scale)
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

        // Keep this in sync with the value used in Traits::createTextPath
        tabStopDistance: Math.round(fontMetrics.advanceWidth("x") * 8)
        // QPainter::drawText doesn't support rich text.
        // We could consider using QStaticText to add rich text support.
        // We probably shouldn't use QTextDocument because that's unnecessarily heavy.
        textFormat: TextEdit.PlainText
        // Keep in sync with Traits::Text::textFlags
        horizontalAlignment: TextEdit.AlignLeft
        verticalAlignment: TextEdit.AlignTop
        wrapMode: TextEdit.NoWrap

        // QPainter uses native antialiasing
        renderType: TextEdit.NativeRendering

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

        leftInset: -background.strokeWidth
        rightInset: -background.strokeWidth
        topInset: -background.strokeWidth
        bottomInset: -background.strokeWidth
        background: DashedOutline {
            pathHints: ShapePath.PathLinear
            strokeWidth: QmlUtils.clampPx(dprRound(1) / root.viewport.scale)
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
                    let dx = dprRound(activeTranslation.x) / viewport.scale
                    let dy = dprRound(activeTranslation.y) / viewport.scale
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
        TextContextMenuConnection {
            target: textField
        }
        Component.onCompleted: forceActiveFocus()
    }
}

