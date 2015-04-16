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
