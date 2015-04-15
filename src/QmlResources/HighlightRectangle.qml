import QtQuick 2.4

Rectangle {
    color: "yellow"
    opacity: 0.35

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

    MouseArea {
        id: resizeNArea

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 5
        anchors.rightMargin: 5
        height: 5

        cursorShape: Qt.SizeVerCursor

        property int initialY : 0
        property int initialH : 0

        onPressed: {
            initialY = parent.y;
            initialH = parent.height;
        }

        onPositionChanged: {
            parent.y = mouse.y;
            if (initialY < mouse.y) {
                if (mouse.y < (initialY + parent.height)) {
                    parent.y = mouse.y;
                    parent.height = initialH - Math.abs(initialY - mouse.y);
                }
            }
        }
    }

    MouseArea {
        id: resizeSArea

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 5
        anchors.rightMargin: 5
        height: 5

        cursorShape: Qt.SizeVerCursor
    }

    MouseArea {
        id: resizeEArea

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.topMargin: 5
        anchors.bottomMargin: 5
        width: 5

        cursorShape: Qt.SizeHorCursor
    }

    MouseArea {
        id: resizeWArea

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.topMargin: 5
        anchors.bottomMargin: 5
        width: 5

        cursorShape: Qt.SizeHorCursor
    }
}
