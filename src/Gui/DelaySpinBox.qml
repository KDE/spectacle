/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQml 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Templates 2.15 as T
import org.kde.kirigami 2.19 as Kirigami
import org.kde.spectacle.private 1.0

/**
 * Meant to mimic the behavior of SmartSpinBox, a custom Qt Widget spinbox created for Spectacle.
 */
QQC2.SpinBox {
    id: root
    readonly property string noDelayString: i18nc("0 second delay", "No Delay")

    function getDisplayText(value) {
        if (value === 0) {
            return noDelayString
        }
        let suffix = ""
        if (value % 100 === 0) {
            suffix = i18ncp("Integer number of seconds",
                        " second", " seconds", value / 100)
        } else {
            suffix = i18nc("Decimal number of seconds", " seconds")
        }
        return textFromValue(value) + suffix
    }

    // QQC2 Spinbox uses ints for some reason,
    // so I need to multiply by 10^decimals to get n decimal places of precision.
    from: 0
    to: 9900 // not 9999 to prevent stepping up by 1 at 99 from going to 99.99
    stepSize: 100
    value: Settings.captureDelay * 100
    onValueModified: Settings.captureDelay = value / 100
    onValueChanged: contentItem.text = getDisplayText(value)

    Connections {
        target: Settings
        function onCaptureDelayChanged() {
            root.value = Settings.captureDelay * 100
        }
    }

    validator: DoubleValidator {
        locale: root.locale.name
        bottom: root.from
        top: root.to / 100
        decimals: 2
        notation: DoubleValidator.StandardNotation
    }

    textFromValue: (value, locale = root.locale) => {
        let precision = 2
        if (value % 100 === 0) {
            precision = 0
        } else if (value % 10 === 0) {
            precision = 1
        }
        return Number(value / 100).toLocaleString(locale, 'f', precision)
    }

    valueFromText: (text, locale = root.locale) => {
        if (text === noDelayString) {
            return 0
        }
        let decimal = locale.decimalPoint
        if (decimal === '.') {
            decimal = "\\."
        }
        let t = text.replace(RegExp(`[^0-9${decimal}]+`, 'g'), ' ')
        let filtered = t.match(RegExp(`\\d{0,2}${decimal}\\d{1,2}`, 'g'))
        if (filtered === null) {
            filtered = t.match(/\d{1,2}/g)
        }
        if (filtered === null) {
            return -1 // unaccepted input
        }
        return Number.fromLocaleString(locale, filtered[0]) * 100
    }

    contentItem: T.TextField {
        text: root.getDisplayText(root.value)
        color: palette.text
        selectionColor: palette.highlight
        selectedTextColor: palette.highlightedText
        selectByMouse: true
        readOnly: !root.editable
        validator: root.validator
        inputMethodHints: root.inputMethodHints
        leftPadding: fontMetrics.descent
        rightPadding: fontMetrics.descent
        horizontalAlignment: TextInput.AlignLeft
        verticalAlignment: TextInput.AlignVCenter
        implicitWidth: {
            const noDelay = fontMetrics.boundingRect(root.noDelayString).width
            const largest = fontMetrics.boundingRect(root.getDisplayText(9999)).width
            return Math.max(noDelay, largest) + leftPadding + rightPadding
        }
        implicitHeight: fontMetrics.height
        onTextEdited: {
            validator.changed()
            if (cursorPosition > 2) {
                const pos1 = cursorPosition - 1
                const char1 = text[pos1]
                if (char1 === '.' || char1 === ',') {
                    const pos2 = cursorPosition - 2
                    const char2 = text[pos2]
                    if (char2 === '.' || char2 === ',') {
                        remove(pos2, pos1)
                    } else {
                        let index = text.indexOf('.')
                        if (index === -1) {
                            index = text.indexOf(',')
                        }
                        if (index !== -1 && index < pos2) {
                            remove(index, pos1)
                        }
                    }
                }
            }
            let seconds = root.valueFromText(text) / 100
            if (acceptableInput || seconds >= 0) {
                const oldCursorPos = cursorPosition
                Settings.captureDelay = seconds
                cursorPosition = oldCursorPos
            }
        }
    }

    FontMetrics {
        id: fontMetrics
    }
}
