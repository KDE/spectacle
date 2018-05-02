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
import QtQuick.Layouts 1.3
import QtQuick.Controls 1.4

Item {
    id: editorRoot;
    objectName: "editorRoot";

    // properties and setters

    property var selection: undefined;
    property color maskColour: Qt.rgba(0, 0, 0, 0.15);
    property color strokeColour: systemPalette.highlight;
    property color crossColour: Qt.rgba(strokeColour.r, strokeColour.g, strokeColour.b, 0.7);
    property color labelBackgroundColour: Qt.rgba(systemPalette.light.r, systemPalette.light.g, systemPalette.light.b, 0.85);
    property bool showMagnifier: false;
    property bool toggleMagnifier: false;
    property int magZoom: 5;
    property int magPixels: 16;
    property int magOffset: 32;

    SystemPalette {
        id: systemPalette;
    }

    function setInitialSelection(xx, yy, ww, hh) {
        if (selection) {
            selection.destroy();
        }

        selection = cropRectangle.createObject(parent, {
             "x":      xx,
             "y":      yy,
             "height": hh,
             "width":  ww
        });

        cropDisplayCanvas.requestPaint();
    }

    function accept() {
        if (selection) {
            acceptImage(selection.x, selection.y, selection.width, selection.height);
        } else {
            acceptImage(-1, -1, -1, -1);
        }
    }

    // key handlers

    focus: true;

    Keys.onReturnPressed: accept()
    Keys.onEnterPressed: accept()

    Keys.onEscapePressed: {
        cancelImage();
    }

    Keys.onPressed: {
        if (event.modifiers & Qt.ShiftModifier) {
            toggleMagnifier = true;
            cropDisplayCanvas.requestPaint();
        }
    }

    Keys.onReleased: {
        if (toggleMagnifier && !(event.modifiers & Qt.ShiftModifier)) {
            toggleMagnifier = false;
            cropDisplayCanvas.requestPaint();
        }
    }

    // signals

    signal acceptImage(int x, int y, int width, int height);
    signal cancelImage();

    Image {
        id: imageBackground;
        objectName: "imageBackground";

        source: "image://snapshot/rawimage";
        cache: false;

        height: Window.height / Screen.devicePixelRatio;
        width: Window.width / Screen.devicePixelRatio;
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
            ctx.strokeStyle = strokeColour;
            ctx.fillStyle = maskColour;

            // draw a sheet over the whole screen
            ctx.fillRect(0, 0, cropDisplayCanvas.width, cropDisplayCanvas.height);

            if (selection) {
                midHelpText.visible = false;
                // display bottom help text only if it does not intersect with the selection
                bottomHelpText.visible = (selection.y + selection.height < bottomHelpText.y) || (selection.x > bottomHelpText.x + bottomHelpText.width) || (selection.x + selection.width < bottomHelpText.x);

                // if we have a selection polygon, cut it out
                ctx.fillStyle = strokeColour;
                ctx.fillRect(selection.x, selection.y, selection.width, selection.height);
                ctx.clearRect(selection.x + 1, selection.y + 1, selection.width - 2, selection.height - 2);

                if ((selection.width > 20) && (selection.height > 20)) {
                    // top-left handle
                    ctx.beginPath();
                    ctx.arc(selection.x, selection.y, 8, 0, 0.5 * Math.PI);
                    ctx.lineTo(selection.x, selection.y);
                    ctx.fill();

                    // top-right handle
                    ctx.beginPath();
                    ctx.arc(selection.x + selection.width, selection.y, 8, 0.5 * Math.PI, Math.PI);
                    ctx.lineTo(selection.x + selection.width, selection.y);
                    ctx.fill();

                    // bottom-left handle
                    ctx.beginPath();
                    ctx.arc(selection.x + selection.width, selection.y + selection.height, 8, Math.PI, 1.5 * Math.PI);
                    ctx.lineTo(selection.x + selection.width, selection.y + selection.height);
                    ctx.fill();

                    // bottom-right handle
                    ctx.beginPath();
                    ctx.arc(selection.x, selection.y + selection.height, 8, 1.5 * Math.PI, 2 * Math.PI);
                    ctx.lineTo(selection.x, selection.y + selection.height);
                    ctx.fill();

                    // top-center handle
                    ctx.beginPath();
                    ctx.arc(selection.x + selection.width / 2, selection.y, 5, 0, Math.PI);
                    ctx.fill();

                    // right-center handle
                    ctx.beginPath();
                    ctx.arc(selection.x + selection.width, selection.y + selection.height / 2, 5, 0.5 * Math.PI, 1.5 * Math.PI);
                    ctx.fill();

                    // bottom-center handle
                    ctx.beginPath();
                    ctx.arc(selection.x + selection.width / 2, selection.y + selection.height, 5, Math.PI, 2 * Math.PI);
                    ctx.fill();

                    // left-center handle
                    ctx.beginPath();
                    ctx.arc(selection.x, selection.y + selection.height / 2, 5, 1.5 * Math.PI, 0.5 * Math.PI);
                    ctx.fill();
                }

                // Set the selection size and finds the most appropriate position:
                // - vertically centered inside the selection if the box is not covering the a large part of selection
                // - on top of the selection if the selection x position fits the box height plus some margin
                // - at the bottom otherwise
                // Note that text is drawn starting from the left bottom!
                var selectionText = Math.round(selection.width * Screen.devicePixelRatio) + "x" + Math.round(selection.height * Screen.devicePixelRatio);
                selectionTextMetrics.font = ctx.font;
                selectionTextMetrics.text = selectionText;
                var selectionTextRect = selectionTextMetrics.boundingRect;
                var selectionBoxX = Math.max(0, selection.x + (selection.width - selectionTextRect.width) / 2);
                var selectionBoxY;
                if ((selection.width > 100) && (selection.height > 100)) {
                    // show inside the box
                    selectionBoxY = selection.y + (selection.height + selectionTextRect.height) / 2;
                } else if (selection.y >= selectionTextRect.height + 8) {
                    // show on top
                    selectionBoxY = selection.y - 8;
                } else {
                    // show at the bottom
                    selectionBoxY = selection.y + selection.height + selectionTextRect.height + 4;
                }
                // Now do the actual box, border and text drawing
                ctx.fillStyle = systemPalette.window;
                ctx.strokeStyle = systemPalette.windowText;
                ctx.fillRect(selectionBoxX - 4, selectionBoxY - selectionTextRect.height - 2, selectionTextRect.width + 10, selectionTextRect.height + 8);
                ctx.strokeRect(selectionBoxX - 4, selectionBoxY - selectionTextRect.height - 2, selectionTextRect.width + 10, selectionTextRect.height + 8);
                ctx.fillStyle = systemPalette.windowText;
                ctx.fillText(selectionText, selectionBoxX, selectionBoxY);
                if (selection.zoomCenterX >= 0 && selection.zoomCenterY >= 0 && (showMagnifier ^ toggleMagnifier)) {
                    var offsetX = magOffset;
                    var offsetY = magOffset;
                    var magX = selection.zoomCenterX;
                    var magY = selection.zoomCenterY;
                    var magWidth = crossMagnifier.width;
                    var magHeight = crossMagnifier.height;

                    if (magX + offsetX + magWidth >= Window.width / Screen.devicePixelRatio) {
                        offsetX -= offsetX * 2 + magWidth;
                    }

                    if (magY + offsetY + magHeight >= Window.height / Screen.devicePixelRatio) {
                        offsetY -= offsetY * 2 + magHeight;
                    }

                    magX += offsetX;
                    magY += offsetY;
                    crossMagnifier.visible = true;
                    crossMagnifier.x = magX;
                    crossMagnifier.y = magY;
                    crossBackground.x = -selection.zoomCenterX * Screen.devicePixelRatio * magZoom + magPixels * magZoom;
                    crossBackground.y = -selection.zoomCenterY * Screen.devicePixelRatio * magZoom + magPixels * magZoom;
                    ctx.strokeRect(magX, magY, magWidth, magHeight);
                } else {
                    crossMagnifier.visible = false;
                }
            } else {
                midHelpText.visible = true;
                bottomHelpText.visible = false;
                crossMagnifier.visible = false;
            }
        }

        TextMetrics {
            id: selectionTextMetrics
        }

        Rectangle {
            id: midHelpText;
            objectName: "midHelpText";

            height: midHelpTextElement.height + 40;
            width: midHelpTextElement.width + 40;
            radius: 4;
            border.color: systemPalette.windowText;
            color: labelBackgroundColour;

            visible: false;
            anchors.centerIn: parent;

            Label {
                id: midHelpTextElement;
                text: i18n("Click anywhere to start drawing a selection rectangle,\n" +
                           "or press Esc to cancel.");
                font.pixelSize: Qt.application.font.pixelSize * 1.2;

                anchors.centerIn: parent;
            }
        }

        Rectangle {
            id: bottomHelpText;
            objectName: "bottomHelpText";

            height: bottomHelpTextElement.height + 20;
            width: bottomHelpTextElement.width + 20;
            radius: 4;
            border.color: systemPalette.windowText;
            color: labelBackgroundColour;

            visible: false;
            anchors.bottom: parent.bottom;
            anchors.horizontalCenter: parent.horizontalCenter;

            GridLayout {
                id: bottomHelpTextElement;
                columns: 2
                anchors.centerIn: parent;

                Label {
                    text: i18n("Enter, double-click:");
                    Layout.alignment: Qt.AlignRight;
                }
                Label { text: i18n("Take screenshot"); }

                Label {
                    text: i18n("Shift:");
                    Layout.alignment: Qt.AlignRight;
                }
                Label { text: i18n("Hold to toggle magnifier"); }

                Label {
                    text: i18n("Right-click:");
                    Layout.alignment: Qt.AlignRight;
                }
                Label { text: i18n("Reset selection"); }

                Label {
                    text: i18n("Esc:");
                    Layout.alignment: Qt.AlignRight;
                }
                Label { text: i18n("Cancel"); }
            }
        }

        // Use Rectangle so that the background is white when cursor nearby edge
        Rectangle {
            id: crossMagnifier;

            height: (magPixels * 2 + 1) * magZoom;
            width: height;
            border.width: 0;
            visible: false;
            clip: true

            Image {
                id: crossBackground;
                source: "image://snapshot/rawimage";
                smooth: false;
                height: Window.height * magZoom;
                width: Window.width * magZoom;
            }

            Rectangle {
                x: magPixels * magZoom;
                y: 0;
                width: magZoom;
                height: magPixels * magZoom;
                color: crossColour;
            }

            Rectangle {
                x: magPixels * magZoom;
                y: (magPixels + 1) * magZoom;
                width: magZoom;
                height: magPixels * magZoom;
                color: crossColour;
            }

            Rectangle {
                x: 0;
                y: magPixels * magZoom;
                width: magPixels * magZoom;
                height: magZoom;
                color: crossColour;
            }

            Rectangle {
                x: (magPixels + 1) * magZoom;
                y: magPixels * magZoom;
                width: magPixels * magZoom;
                height: magZoom;
                color: crossColour;
            }
        }

    }

    MouseArea {
        anchors.fill: imageBackground;

        property int startx: 0;
        property int starty: 0;

        cursorShape: Qt.CrossCursor;
        acceptedButtons: Qt.LeftButton | Qt.RightButton;

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
            selection.width = Math.abs(startx - mouse.x) + 1;
            selection.height = Math.abs(starty - mouse.y) + 1;
            selection.zoomCenterX = mouse.x;
            selection.zoomCenterY = mouse.y;
            cropDisplayCanvas.requestPaint();
        }

        onClicked: {
            if ((mouse.button == Qt.RightButton) && (selection)) {
                selection.destroy();
                cropDisplayCanvas.requestPaint();
            }
        }

        onReleased: {
            selection.zoomCenterX = -1;
            selection.zoomCenterY = -1;
            cropDisplayCanvas.requestPaint();
        }
    }

    Component {
        id: cropRectangle;

        SelectionRectangle {
            drawCanvas: cropDisplayCanvas;
            imageElement: imageBackground;

            onDoubleClicked: editorRoot.accept()
        }
    }
}
