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
import QtQuick.Window 2.2

Item {
    id: editorRoot;
    objectName: "editorRoot";

    property var selection: undefined;

    signal acceptImage(int x, int y, int width, int height);
    signal cancelImage();

    Image {
        id: imageBackground;
        objectName: "imageBackground";

        source: "image://snapshot/rawimage";
        cache: false;

        height: Screen.height;
        width: Screen.width;
        fillMode: Image.PreserveAspectFit;
    }

    Canvas {
        id: cropDisplayCanvas;
        objectName: "cropDisplayCanvas";
        anchors.fill: imageBackground;

        renderTarget: Canvas.FramebufferObject;
        renderStrategy: Canvas.Cooperative;

        onPaint: {
            // start by getting a context on the canvas and clearing it
            var ctx = cropDisplayCanvas.getContext("2d");
            ctx.clearRect(0, 0, cropDisplayCanvas.width, cropDisplayCanvas.height);

            // set up the colours
            ctx.strokeStyle = Qt.rgba(0, 0, 0, 1);
            ctx.fillStyle = Qt.rgba(0, 0, 0, 0.75);

            // draw a sheet over the whole screen
            ctx.fillRect(0, 0, cropDisplayCanvas.width, cropDisplayCanvas.height);

            // if we have a selection polygon, cut it out
            if (selection) {
                ctx.clearRect(selection.x, selection.y, selection.width, selection.height);
                ctx.strokeRect(selection.x, selection.y, selection.width, selection.height);

                if ((selection.width > 20) && (selection.height > 20)) {
                    ctx.fillStyle = Qt.rgba(0, 0, 0, 1);

                    // top-left handle
                    ctx.beginPath();
                    ctx.arc(selection.x, selection.y, 8, 0, 0.5 * Math.PI);
                    ctx.lineTo(selection.x, selection.y);
                    ctx.fill();
                    ctx.stroke();

                    // top-right handle
                    ctx.beginPath();
                    ctx.arc(selection.x + selection.width, selection.y, 8, 0.5 * Math.PI, Math.PI);
                    ctx.lineTo(selection.x + selection.width, selection.y);
                    ctx.fill();
                    ctx.stroke();

                    // bottom-left handle
                    ctx.beginPath();
                    ctx.arc(selection.x + selection.width, selection.y + selection.height, 8, Math.PI, 1.5 * Math.PI);
                    ctx.lineTo(selection.x + selection.width, selection.y + selection.height);
                    ctx.fill();
                    ctx.stroke();

                    // bottom-right handle
                    ctx.beginPath();
                    ctx.arc(selection.x, selection.y + selection.height, 8, 1.5 * Math.PI, 2 * Math.PI);
                    ctx.lineTo(selection.x, selection.y + selection.height);
                    ctx.fill();
                    ctx.stroke();

                    // top-center handle
                    ctx.beginPath();
                    ctx.arc(selection.x + selection.width / 2, selection.y, 5, 0, Math.PI);
                    ctx.fill();
                    ctx.stroke();

                    // right-center handle
                    ctx.beginPath();
                    ctx.arc(selection.x + selection.width, selection.y + selection.height / 2, 5, 0.5 * Math.PI, 1.5 * Math.PI);
                    ctx.fill();
                    ctx.stroke();

                    // bottom-center handle
                    ctx.beginPath();
                    ctx.arc(selection.x + selection.width / 2, selection.y + selection.height, 5, Math.PI, 2 * Math.PI);
                    ctx.fill();
                    ctx.stroke();

                    // left-center handle
                    ctx.beginPath();
                    ctx.arc(selection.x, selection.y + selection.height / 2, 5, 1.5 * Math.PI, 0.5 * Math.PI);
                    ctx.fill();
                    ctx.stroke();
                }
            }
        }
    }

    MouseArea {
        anchors.fill: imageBackground;

        property int startx: 0;
        property int starty: 0;

        cursorShape: Qt.CrossCursor;

        onPressed: {
            if (selection) {
                selection.destroy();
            }

            startx = mouse.x;
            starty = mouse.y;

            selection = cropRectangle.createObject(parent, {
                 "x": startx,
                 "y": starty,
                 "height": 0,
                 "width": 0
            });
        }

        onPositionChanged: {
            selection.x = Math.min(startx, mouse.x);
            selection.y = Math.min(starty, mouse.y);
            selection.width = Math.abs(startx - mouse.x);
            selection.height = Math.abs(starty - mouse.y);

            cropDisplayCanvas.requestPaint();
        }
    }

    Component {
        id: cropRectangle;

        SelectionRectangle {
            drawCanvas: cropDisplayCanvas;
            imageElement: imageBackground;

            onDoubleClicked: {
                editorRoot.acceptImage(selection.x, selection.y, selection.width, selection.height);
            }
        }
    }
}
