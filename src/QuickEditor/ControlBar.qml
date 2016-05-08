import QtQuick 2.5
import QtQuick.Layouts 1.3
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

Item {
    property color bgColor: Qt.rgba(0.85, 0.85, 0.85, 1);

    // signals
    signal cropToolSelected();
    signal rectToolSelected();

    // toolbar
    Rectangle {
        id: toolSelectorBar;
        objectName: "toolSelectorBar";
        border.color: "black";
        border.width: 2;

        height: 40;
        radius: 20;
        width: 384;
        color: bgColor;

        anchors.left: colorSelectorCircle.right;
        anchors.verticalCenter: colorSelectorCircle.verticalCenter;
        anchors.leftMargin: -32;

        RowLayout {
            anchors.verticalCenter: parent.verticalCenter;

            Rectangle {
                color: Qt.rgba(0, 0, 0, 0);
                width: 32;
                height: 1;
            }

            ToolButton {
                exclusiveGroup: toolSelectionGroup;
                action: toolCrop;
            }

            ToolButton {
                exclusiveGroup: toolSelectionGroup;
                action: toolRect;
            }
        }
    }

    Rectangle {
        id: colorSelectorCircle;
        objectName: "colorSelectorCircle";
        border.color: "black";
        border.width: 2;

        property int side: 64;

        height: side;
        width:  side;
        radius: side / 2;

        color: bgColor;
    }

    ExclusiveGroup {
        id: toolSelectionGroup;
        objectName: "toolSelectionGroup";

        Action { id: toolCrop; checkable: true; text: i18n("Crop"); iconName: "transform-crop"; onTriggered: { cropToolSelected(); } }
        Action { id: toolRect; checkable: true; text: i18n("Rectangle"); iconName: "draw-rectangle"; onTriggered: { rectToolSelected(); } }
    }
}
