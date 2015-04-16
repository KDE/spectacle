/*
 *  Copyright (C) 2015 Boudhayan Gupta <me@BaloneyGeek.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

import QtQuick 2.4
import QtQuick.Controls 1.3

Item {
    id: selectionContainer;

    signal selectionCancelled;
    signal selectionConfirmed(int x, int y, int width, int height);

    Rectangle {
        id: selectionRectangle;

        color: "yellow";
        opacity: 0.5;

        anchors.centerIn: parent;
        anchors.fill: parent;
    }

    MouseArea {
        id: moveArea

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 5

        cursorShape: Qt.OpenHandCursor

        drag.target: parent
        drag.axis: Drag.XAndYAxis
        drag.minimumX: 0
        drag.minimumY: 0
        drag.maximumX: parent.parent.width - parent.width
        drag.maximumY: parent.parent.height - parent.height

        onPressed: {
            cursorShape = Qt.ClosedHandCursor;
        }

        onReleased: {
            cursorShape = Qt.OpenHandCursor;
        }
    }

    Row {
        id: buttonStore;
        anchors.centerIn: selectionRectangle;

        opacity: 0.9;

        Button { action: exitAction; }
        Button { action: doneAction; }
    }

    Action {
        id: exitAction;
        text: i18n("Cancel");
        iconName: "dialog-close";
        enabled: true;
        onTriggered: selectionCancelled();
    }

    Action {
        id: doneAction;
        text: i18n("Confirm");
        iconName: "dialog-ok";
        enabled: true;
        onTriggered: selectionConfirmed(selectionContainer.x, selectionContainer.y, selectionContainer.width, selectionContainer.height);
    }
}
