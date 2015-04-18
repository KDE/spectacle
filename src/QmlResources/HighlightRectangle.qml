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

    // resize helper functions

    property int initialTop;
    property int initialLeft;
    property int initialWidth;
    property int initialHeight;

    function saveInitials() {
        selectionContainer.initialHeight = selectionContainer.height;
        selectionContainer.initialWidth  = selectionContainer.width;
        selectionContainer.initialTop    = selectionContainer.y;
        selectionContainer.initialLeft   = selectionContainer.x;
    }

    function resizeFromTop(curMouseY, oldMouseY) {
        var newY = selectionContainer.y + (curMouseY - oldMouseY);
        var newHeight = selectionContainer.initialHeight - (newY - selectionContainer.initialTop);

        if ((newY >= 0) && (newHeight > 0)) {
            selectionContainer.y = newY;
            selectionContainer.height = newHeight;
        }
    }

    function resizeFromLeft(curMouseX, oldMouseX) {
        var newX = selectionContainer.x + (curMouseX - oldMouseX);
        var newWidth = selectionContainer.initialWidth - (newX - selectionContainer.initialLeft);

        if ((newX >= 0) && (newWidth > 0)) {
            selectionContainer.x = newX;
            selectionContainer.width = newWidth;
        }
    }

    function resizeFromBottom(curMouseY, oldMouseY) {
        var newHeight = selectionContainer.height + (curMouseY - oldMouseY);
        if (newHeight > 0) {
            selectionContainer.height = newHeight;
        }
    }

    function resizeFromRight(curMouseX, oldMouseX) {
        var newWidth = selectionContainer.width + (curMouseX - oldMouseX);
        if (newWidth > 0) {
            selectionContainer.width = newWidth;
        }
    }

    // the selection rectangle and its mouse area

    Rectangle {
        id: selectionRectangle;

        property int sizeGripBorderWidth: 8;

        color: Qt.rgba(1, 0.9, 0, 0.5);
        border.color: Qt.rgba(1, 0.9, 0, 0.7);
        border.width: 3;

        anchors.centerIn: parent;
        anchors.fill: parent;
    }

    MouseArea {
        id: moveArea

        anchors.centerIn: parent;
        height: parent.height - (2 * selectionRectangle.sizeGripBorderWidth);
        width:  parent.width - (2 * selectionRectangle.sizeGripBorderWidth);

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

    // resize grips

    MouseArea {
        id: sizeGripTopMouseArea;
        cursorShape: Qt.SizeVerCursor;

        anchors.top: parent.top;
        anchors.horizontalCenter: parent.horizontalCenter;

        height: selectionRectangle.sizeGripBorderWidth;
        width:  parent.width - (2 * selectionRectangle.sizeGripBorderWidth);

        property int oldMouseY;

        onPressed: {
            oldMouseY = mouseY;
            saveInitials();
        }

        onPositionChanged: {
            resizeFromTop(mouseY, oldMouseY);
        }
    }

    MouseArea {
        id: sizeGripLeftMouseArea;
        cursorShape: Qt.SizeHorCursor;

        anchors.left: parent.left;
        anchors.verticalCenter: parent.verticalCenter;

        height: parent.height - (2 * selectionRectangle.sizeGripBorderWidth);
        width:  selectionRectangle.sizeGripBorderWidth;

        property int oldMouseX;

        onPressed: {
            oldMouseX = mouseX;
            saveInitials();
        }

        onPositionChanged: {
            resizeFromLeft(mouseX, oldMouseX);
        }
    }

    MouseArea {
        id: sizeGripRightMouseArea;
        cursorShape: Qt.SizeHorCursor;

        anchors.right: parent.right;
        anchors.verticalCenter: parent.verticalCenter;

        height: parent.height - (2 * selectionRectangle.sizeGripBorderWidth);
        width:  selectionRectangle.sizeGripBorderWidth;

        property int oldMouseX;

        onPressed: {
            oldMouseX = mouseX;
        }

        onPositionChanged: {
            resizeFromRight(mouseX, oldMouseX);
        }
    }

    MouseArea {
        id: sizeGripBottomMouseArea;
        cursorShape: Qt.SizeVerCursor;

        anchors.bottom: parent.bottom;
        anchors.horizontalCenter: parent.horizontalCenter;

        height: selectionRectangle.sizeGripBorderWidth;
        width:  parent.width - (2 * selectionRectangle.sizeGripBorderWidth);

        property int oldMouseY;

        onPressed: {
            oldMouseY = mouseY;
        }

        onPositionChanged: {
            resizeFromBottom(mouseY, oldMouseY);
        }
    }

    Rectangle {
        height: 20;
        width:  20;
        color:  "transparent";

        anchors.horizontalCenter: parent.left;
        anchors.verticalCenter:   parent.top;

        MouseArea {
            id: sizeGripTopLeftMouseArea
            anchors.fill: parent;
            cursorShape: Qt.SizeFDiagCursor;

            property int oldMouseX;
            property int oldMouseY;

            onPressed: {
                oldMouseX = mouseX;
                oldMouseY = mouseY;
                saveInitials();
            }

            onPositionChanged: {
                resizeFromLeft(mouseX, oldMouseX);
                resizeFromTop(mouseY, oldMouseY);
            }
        }
    }

    Rectangle {
        height: 20;
        width:  20;
        color:  "transparent";

        anchors.horizontalCenter: parent.right;
        anchors.verticalCenter:   parent.top;

        MouseArea {
            id: sizeGripTopRightMouseArea
            anchors.fill: parent;
            cursorShape: Qt.SizeBDiagCursor;

            property int oldMouseX;
            property int oldMouseY;

            onPressed: {
                oldMouseX = mouseX;
                oldMouseY = mouseY;
                saveInitials();
            }

            onPositionChanged: {
                resizeFromRight(mouseX, oldMouseX);
                resizeFromTop(mouseY, oldMouseY);
            }
        }
    }

    Rectangle {
        height: 20;
        width:  20;
        color:  "transparent";

        anchors.horizontalCenter: parent.left;
        anchors.verticalCenter:   parent.bottom;

        MouseArea {
            id: sizeGripBottomLeftMouseArea
            anchors.fill: parent;
            cursorShape: Qt.SizeBDiagCursor;

            property int oldMouseX;
            property int oldMouseY;

            onPressed: {
                oldMouseX = mouseX;
                oldMouseY = mouseY;
                saveInitials();
            }

            onPositionChanged: {
                resizeFromLeft(mouseX, oldMouseX);
                resizeFromBottom(mouseY, oldMouseY);
            }
        }
    }

    Rectangle {
        height: 20;
        width:  20;
        color:  "transparent";

        anchors.horizontalCenter: parent.right;
        anchors.verticalCenter:   parent.bottom;

        MouseArea {
            id: sizeGripBottomRightMouseArea
            anchors.fill: parent;
            cursorShape: Qt.SizeFDiagCursor;

            property int oldMouseX;
            property int oldMouseY;

            onPressed: {
                oldMouseX = mouseX;
                oldMouseY = mouseY;
            }

            onPositionChanged: {
                resizeFromRight(mouseX, oldMouseX);
                resizeFromBottom(mouseY, oldMouseY);
            }
        }
    }

    // the buttons inside the rectangle

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
