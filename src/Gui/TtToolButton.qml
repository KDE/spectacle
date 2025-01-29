/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC
import org.kde.kirigami as Kirigami
import org.kde.spectacle.private

QQC.ToolButton {
    implicitHeight: QmlUtils.iconTextButtonHeight
    width: display === QQC.ToolButton.IconOnly ? height : implicitWidth
    QQC.ToolTip.text: text
    QQC.ToolTip.visible: (hovered || pressed) && display === QQC.ToolButton.IconOnly
    QQC.ToolTip.delay: Kirigami.Units.toolTipDelay
}
