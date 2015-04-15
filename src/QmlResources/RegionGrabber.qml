import QtQuick 2.4
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.3

import "HoverBar"
import "HighlightRectangle"

Canvas {
    id: canvas

    property string imagefile : "image://screenshot/image"
    property string selectmode : ""

    Component.onCompleted: loadImage(canvas.imagefile)
    onImageLoaded: reset()

    onPaint: {
    }

    function init() {
    }

    function setMode(mode) {
        canvas.reset();
        canvas.selectmode = mode;

        selectArea.reset();
        selectArea.selectmode = mode;

        if (mode == "rectangle") {
            // do something
        } else if (mode == "polygon") {
            // do something else
        } else if (mode == "brush") {
            // do what you want to
        }
    }

    function reset() {
        var context = canvas.getContext("2d");
        context.clearRect(0, 0, canvas.width, canvas.height);
        context.drawImage(canvas.imagefile, 0, 0);
        canvas.requestPaint();
    }

    function done() {
        if (selectArea.highlightItem === null) {
            return;
        }

        var x = selectArea.highlightItem.x;
        var y = selectArea.highlightItem.y;
        var w = selectArea.highlightItem.width;
        var h = selectArea.highlightItem.height;
        selectArea.highlightItem.destroy();

        var context = canvas.getContext("2d");

        context.clearRect(0, 0, canvas.width, canvas.height);
        context.save();
        context.beginPath();
        context.rect(x, y, w, h);
        context.clip();
        context.drawImage(canvas.imagefile, 0, 0);
        context.restore();

        canvas.requestPaint();
    }

    function crop() {
        var x = selectArea.highlightItem.x;
        var y = selectArea.highlightItem.y;
        var w = selectArea.highlightItem.width;
        var h = selectArea.highlightItem.height;
    }

    /*
    Canvas {
        anchors.fill: parent;
        id: drawCanvas;

        Component.onCompleted: requestPaint()

        onPaint: {
            drawCanvas.paintSelectionRectangle(0, 0, 150, 150);

        }

        function paintSelectionRectangle(x, y, w, h) {
            var context = drawCanvas.getContext("2d");
            context.clearRect(0, 0, drawCanvas.width, drawCanvas.height);

            context.beginPath();
            context.rect(x, y, w, h);
            context.strokeStyle = "yellow";
            context.fillStyle = "yellow";
            context.globalAlpha = 0.5;
            context.stroke();
            context.globalAlpha = 0.35;
            context.fill();
        }

        MouseArea {
            id: selectArea
            anchors.fill: parent
            cursorShape: Qt.CrossCursor

            property int initialX : 0
            property int initialY : 0

            onPressed: {


        }
    }
    */

    MouseArea {
        id: selectArea
        anchors.fill: parent
        cursorShape: Qt.CrossCursor

        property variant highlightItem : null;
        property string selectmode : null

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
            id: highlightBrush

            Rectangle {
                color: "yellow"
                opacity: 0.35

                width: (canvas.width / 100)
                height: width
                radius: (width / 2)
            }
        }

        Component {
            id: highlightRectangle
            HighlightRectangle {}
        }
    }

    HoverBar { canvas: canvas }
}
