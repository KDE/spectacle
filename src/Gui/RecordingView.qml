/*
 * SPDX-FileCopyrightText: 2023 Aleix Pol i Gonzalez <aleixpol@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtMultimedia 5.15
import org.kde.kirigami 2.19 as Kirigami
import org.kde.spectacle.private 1.0

import "Annotations"

Item
{
    implicitWidth: parent.width
    implicitHeight: parent.height

    Video {
        anchors.fill: parent
        source: SpectacleCore.currentVideo
        flushMode: VideoOutput.FirstFrame
        autoPlay: true

        Text {
            text: parent.availability
            anchors.fill: parent
            opacity: 0.3
        }
    }

    Kirigami.Heading {
        anchors.centerIn: parent
        visible: SpectacleCore.isRecording
        text: i18n("Recording") + " " + parent.width + "x" + parent.height
    }
}
