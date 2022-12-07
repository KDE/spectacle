/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import org.kde.kirigami 2.19 as Kirigami

Kirigami.ShadowedRectangle {
    radius: Kirigami.Units.mediumSpacing / 2
    shadow.color: Qt.rgba(0,0,0,0.2)
    shadow.size: 9
    shadow.yOffset: 2
}
