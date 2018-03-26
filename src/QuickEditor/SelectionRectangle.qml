/*
 *  Copyright (C) 2016 Boudhayan Gupta <bgupta@kde.org>
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

import QtQuick 2.5

Item {
    id: cropRectItem;
    objectName: "cropRectItem";

    property var drawCanvas: null;
    property var imageElement: null;
    property int minRectSize: 20;
    property int mouseAreaSize: 20;

    signal doubleClicked();

    onWidthChanged: {
        var maxWidth = imageElement.width - x;
        if (width > maxWidth) {
            width = maxWidth;
        }
    }

    onHeightChanged: {
        var maxHeight = imageElement.height - y;
        if (height > maxHeight) {
            height = maxHeight;
        }
    }

    MouseArea {
        anchors.fill: parent;
        cursorShape: Qt.OpenHandCursor;

        drag.target: parent;
        drag.minimumX: 0;
        drag.maximumX: imageElement.width - parent.width;
        drag.minimumY: 0;
        drag.maximumY: imageElement.height - parent.height;
        drag.smoothed: true;

        onPressed: { cursorShape = Qt.ClosedHandCursor; }
        onPositionChanged: { drawCanvas.requestPaint(); }
        onReleased: { cursorShape = Qt.OpenHandCursor; }
        onDoubleClicked: { cropRectItem.doubleClicked(); }
    }

    MouseArea {
        id: hTopLeft;

        property int brxLimit: 0;
        property int bryLimit: 0;

        anchors.top: parent.top;
        anchors.left: parent.left;

        width: mouseAreaSize;
        height: mouseAreaSize;
        cursorShape: Qt.SizeFDiagCursor;

        onPressed: {
            brxLimit = (parent.x + parent.width) - minRectSize;
            bryLimit = (parent.y + parent.height) - minRectSize;
        }

        onPositionChanged: {
            if ((parent.x + mouse.x) < brxLimit) {
                parent.width = parent.width - mouse.x;
                parent.x = parent.x + mouse.x;
            }

            if ((parent.y + mouse.y) < bryLimit) {
                parent.height = parent.height - mouse.y;
                parent.y = parent.y + mouse.y;
            }

            drawCanvas.requestPaint();
        }
    }

    MouseArea {
        id: hTopRight;

        property int brxLimit: 0;
        property int bryLimit: 0;

        anchors.top: parent.top;
        anchors.right: parent.right;

        width: mouseAreaSize;
        height: mouseAreaSize;
        cursorShape: Qt.SizeBDiagCursor;

        onPressed: {
            brxLimit = parent.x + mouseAreaSize + minRectSize;
            bryLimit = (parent.y + parent.height) - minRectSize;
        }

        onPositionChanged: {
            if ((parent.x + parent.width + mouse.x) > brxLimit) {
                parent.width = parent.width + mouse.x - mouseAreaSize + 1;
            }

            if ((parent.y + mouse.y) < bryLimit) {
                parent.height = parent.height - mouse.y;
                parent.y = parent.y + mouse.y;
            }

            drawCanvas.requestPaint();
        }
    }

    MouseArea {
        id: hBottomLeft;

        property int brxLimit: 0;
        property int bryLimit: 0;

        anchors.bottom: parent.bottom;
        anchors.left: parent.left;

        width: mouseAreaSize;
        height: mouseAreaSize;
        cursorShape: Qt.SizeBDiagCursor;

        onPressed: {
            brxLimit = (parent.x + parent.width) - minRectSize;
            bryLimit = parent.y + mouseAreaSize + minRectSize;
        }

        onPositionChanged: {
            if ((parent.x + mouse.x) < brxLimit) {
                parent.width = parent.width - mouse.x;
                parent.x = parent.x + mouse.x;
            }

            if ((parent.y + parent.height + mouse.y) > bryLimit) {
                parent.height = parent.height + mouse.y - mouseAreaSize + 1;
            }

            drawCanvas.requestPaint();
        }
    }

    MouseArea {
        id: hBottomRight;

        property int brxLimit: 0;
        property int bryLimit: 0;

        anchors.bottom: parent.bottom;
        anchors.right: parent.right;

        width: mouseAreaSize;
        height: mouseAreaSize;
        cursorShape: Qt.SizeFDiagCursor;

        onPressed: {
            brxLimit = parent.x + mouseAreaSize + minRectSize;
            bryLimit = parent.y + mouseAreaSize + minRectSize;
        }

        onPositionChanged: {
            if ((parent.x + parent.width + mouse.x) > brxLimit) {
                parent.width = parent.width + mouse.x - mouseAreaSize + 1;
            }

            if ((parent.y + parent.height + mouse.y) > bryLimit) {
                parent.height = parent.height + mouse.y - mouseAreaSize + 1;
            }
            drawCanvas.requestPaint();
        }
    }

    MouseArea {
        id: hTop;

        property int limit: 0;

        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.top: parent.top;

        width: mouseAreaSize;
        height: mouseAreaSize;
        cursorShape: Qt.SizeVerCursor;

        onPressed: {
            limit = (parent.y + parent.height) - minRectSize;
        }

        onPositionChanged: {
            if ((parent.y + mouse.y) < limit) {
                parent.height = parent.height - mouse.y;
                parent.y = parent.y + mouse.y;
            }

            drawCanvas.requestPaint();
        }
    }

    MouseArea {
        id: hBottom;

        property int limit: 0;

        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.bottom: parent.bottom;

        width: mouseAreaSize;
        height: mouseAreaSize;
        cursorShape: Qt.SizeVerCursor;

        onPressed: {
            limit = parent.y + mouseAreaSize + minRectSize;
        }

        onPositionChanged: {
            if ((parent.y + parent.height + mouse.y) > limit) {
                parent.height = parent.height + mouse.y - mouseAreaSize + 1;
            }

            drawCanvas.requestPaint();
        }
    }

    MouseArea {
        id: hLeft;

        property int limit: 0;

        anchors.verticalCenter: parent.verticalCenter;
        anchors.left: parent.left;

        width: mouseAreaSize;
        height: mouseAreaSize;
        cursorShape: Qt.SizeHorCursor;

        onPressed: {
            limit = (parent.x + parent.width) - minRectSize;
        }

        onPositionChanged: {
            if ((parent.x + mouse.x) < limit) {
                parent.width = parent.width - mouse.x;
                parent.x = parent.x + mouse.x;
            }

            drawCanvas.requestPaint();
        }
    }

    MouseArea {
        id: hRight;

        property int limit: 0;

        anchors.verticalCenter: parent.verticalCenter;
        anchors.right: parent.right;

        width: mouseAreaSize;
        height: mouseAreaSize;
        cursorShape: Qt.SizeHorCursor;

        onPressed: {
            limit = parent.x + mouseAreaSize + minRectSize;
        }

        onPositionChanged: {
            if ((parent.x + parent.width + mouse.x) > limit) {
                parent.width = parent.width + mouse.x - mouseAreaSize + 1;
            }

            drawCanvas.requestPaint();
        }
    }
}
