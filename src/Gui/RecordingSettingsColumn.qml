/* SPDX-FileCopyrightText: 2023 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

ColumnLayout {
    spacing: Kirigami.Units.mediumSpacing
    QQC.CheckBox {
        Layout.fillWidth: true
        text: i18n("Include mouse pointer")
        QQC.ToolTip.text: i18n("Show the mouse cursor in the screen recording.")
        QQC.ToolTip.delay: Kirigami.Units.toolTipDelay
        QQC.ToolTip.visible: hovered
        checked: Settings.videoIncludePointer
        onToggled: Settings.videoIncludePointer = checked
    }
}
