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

    // properties and setters

    property color maskColour: Qt.rgba(0, 0, 0, 0.75);
    property color strokeColour: Qt.rgba(0.114, 0.6, 0.953, 1);

    function setInitialSelection(xx, yy, ww, hh) {
        cropSurface.setInitialSelection(xx, yy, ww, hh);
    }

    function grabImageStart() {
        editorSurface.captureCanvasImage();
    }

    function grabImageDone(drawn) {
        var xx = cropSurface.getSelectionX() * Screen.devicePixelRatio;
        var yy = cropSurface.getSelectionY() * Screen.devicePixelRatio;
        var ww = cropSurface.getSelectionW() * Screen.devicePixelRatio;
        var hh = cropSurface.getSelectionH() * Screen.devicePixelRatio;
        var imgdata = Array.prototype.slice.call(drawn.data);
        acceptImage(xx, yy, ww, hh, imgdata, drawn.width, drawn.height);
    }

    // key handlers

    focus: true;

    Keys.onReturnPressed: {
        grabImageStart();
    }

    Keys.onEnterPressed: {
        grabImageStart();
    }

    Keys.onEscapePressed: {
        cancelImage();
    }

    Keys.onDeletePressed:  {
        editorSurface.deleteSelectedShape();
    }

    // signals

    signal acceptImage(int x, int y, int width, int height, var canvasimg, int cw, int ch);
    signal cancelImage();

    // states

    states: [
        State {
            name: "CropState";
            PropertyChanges { target: cropSurface; interactionActive: true; }
            PropertyChanges { target: editorSurface; interactionActive: false; }
        },
        State {
            name: "RectToolState";
            PropertyChanges { target: cropSurface; interactionActive: false; }
            PropertyChanges { target: editorSurface; interactionActive: true; }
        }
    ]
    state: "CropState";

    // surfaces

    Image {
        id: imageBackground;
        objectName: "imageBackground";

        source: "image://snapshot/rawimage";
        cache: false;

        height: Window.height / Screen.devicePixelRatio;
        width: Window.width / Screen.devicePixelRatio;
        fillMode: Image.PreserveAspectFit;
    }

    DrawSurface {
        id: editorSurface;
        objectName: "editorSurface";
        anchors.fill: imageBackground;

        interactionActive: false;
        onCanvasCaptured: {
            grabImageDone(imageData);
        }
    }

    CropSurface {
        id: cropSurface;
        objectName: "cropSurface";
        anchors.fill: imageBackground;

        interactionActive: true;
        maskColour: editorRoot.maskColour;
        strokeColour: editorRoot.strokeColour;

        onAcceptImage: {
            grabImageStart();
        }
    }

    // manipulation and controls

    ControlBar {
        id: toolSelector;
        objectName: "toolSelector";

        onCropToolSelected: { editorRoot.state = "CropState"; }
        onRectToolSelected: { editorRoot.state = "RectToolState"; }

        //anchors.bottom: imageBackground.bottom;
        //anchors.left: imageBackground.left;
        //anchors.leftMargin: 32;
        //anchors.bottomMargin: -32;
    }
}
