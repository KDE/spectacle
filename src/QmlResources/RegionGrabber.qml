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
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.3

import "qml:///HighlightRectangle"

Item {
    id: rootItem;

    signal selectionCancelled;
    signal selectionConfirmed(int x, int y, int width, int height);

    function loadScreenshot() {
        var rstring = Math.random().toString().substring(4);
        nonLiveScreenshotImage.source = "image://screenshot/" + rstring;
    }

    Image {
        id: nonLiveScreenshotImage;
    }

    MouseArea {
        id: selectArea
        anchors.fill: parent
        cursorShape: Qt.CrossCursor

        property variant highlightItem : null;

        property int initialX : 0
        property int initialY : 0

        function reset() {
        }

        onPressed: {
            if (highlightItem !== null) {
                highlightItem.destroy();
            }

            initialX = mouse.x;
            initialY = mouse.y;

            highlightItem = highlightRectangle.createObject(selectArea, {
                "x" : mouse.x,
                "y" : mouse.y,
            });
        }

        onPositionChanged: {
            if (mouse.x < initialX) {
                highlightItem.x = mouse.x;
                highlightItem.width = (initialX - mouse.x);
            } else {
                highlightItem.width = (Math.abs (mouse.x - highlightItem.x));
            }

            if (mouse.y < initialY) {
                highlightItem.y = mouse.y;
                highlightItem.height = (initialY - mouse.y);
            } else {
                highlightItem.height = (Math.abs (mouse.y - highlightItem.y));
            }
        }

        Component {
            id: highlightRectangle

            HighlightRectangle {
                onSelectionCancelled: rootItem.selectionCancelled();
                onSelectionConfirmed: rootItem.selectionConfirmed(x, y, width, height);
            }
        }
    }
}
