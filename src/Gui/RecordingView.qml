/*
 * SPDX-FileCopyrightText: 2023 Aleix Pol i Gonzalez <aleixpol@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import VLCQt 1.0
import org.kde.kirigami 2.19 as Kirigami
import org.kde.spectacle.private 1.0

import "Annotations"

Item
{
    implicitWidth: parent.width
    implicitHeight: parent.height

    VlcVideoPlayer {
        anchors.fill: parent
        visible: !SpectacleCore.isRecording
        url: SpectacleCore.currentVideo
    }

    Kirigami.Heading {
        anchors.centerIn: parent
        visible: SpectacleCore.isRecording
        text: i18n("Recording") + " " + parent.width + "x" + parent.height
    }
}
