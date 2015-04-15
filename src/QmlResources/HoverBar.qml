import QtQuick 2.4
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.3

ToolBar {
    property Canvas canvas : null

    id: toolBar
    width: (canvas.width / 3)

    anchors.horizontalCenter: canvas.horizontalCenter
    anchors.verticalCenter: canvas.verticalCenter
    anchors.verticalCenterOffset: (canvas.height / 3.5)

    opacity: 0.9

    RowLayout {

        anchors.fill: parent

        MouseArea {
            id: toolBarMouseArea
            anchors.fill: parent
            cursorShape: Qt.ArrowCursor
        }

        Label {
            text: "Selection Mode"
        }

        ComboBox {
            currentIndex: 0
            width: 200

            model: ListModel {
                id: modeSelector
                ListElement { text: "Rectangle"; mode: "rectangle" }
                ListElement { text: "Polygon";   mode: "polygon" }
            }

            onCurrentIndexChanged: {
                if (canvas !==  null) {
                    canvas.setMode(modeSelector.get(currentIndex).mode);
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }

        ToolButton {
            action: resetAction
        }

        ToolButton {
            action: doneAction
        }

        ToolButton {
            action: exitAction
        }
    }

    Action {
        id: exitAction
        text: "&Exit"
        iconName: "dialog-close"
        enabled: true
        onTriggered: Qt.quit()
    }

    Action {
        id: doneAction
        text: "&Done"
        iconName: "dialog-ok"
        enabled: true
        onTriggered: canvas.done()
    }

    Action {
        id: resetAction
        text: "&Reset"
        iconName: "edit-clear"
        enabled: true
        onTriggered: canvas.reset()
    }
}
