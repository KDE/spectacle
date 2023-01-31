/*
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.spectacle.private 1.0

import "Annotations"

AnnotationEditor {
    implicitWidth: contextWindow.imageSize.width / contextWindow.imageDpr
    implicitHeight: contextWindow.imageSize.height / contextWindow.imageDpr
    transformOrigin: Item.TopLeft
    zoom: Math.min(1, Math.max(parent.minZoom, parent.defaultZoom))
    scale: Math.max(1, Math.min(parent.maxZoom, parent.defaultZoom))
    antialiasing: false
    smooth: !contextWindow.annotating || effectiveZoom <= 2
    width: implicitWidth
    height: implicitHeight
    visible: true
    enabled: contextWindow.annotating
        && AnnotationDocument.tool.type !== AnnotationDocument.None
}
