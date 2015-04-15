import QtQuick 2.0
import QtGraphicalEffects 1.0
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1

ColumnLayout {
    id: mainLayout;
    spacing: 10;

    signal newScreenshotRequest(string captureType, real captureDelay, bool includePointer, bool includeDecorations);
    signal saveCheckboxStates(bool includePointer, bool includeDecorations);
    signal saveCaptureMode(int captureModeIndex);
    signal editScreenshotRequest;

    function reloadScreenshot() {
        screenshotImage.refreshImage();
    }

    function loadCheckboxStates(includePointer, includeDecorations) {
        optionMousePointer.checked = includePointer;
        optionWindowDecorations.checked = includeDecorations;
    }

    function loadCaptureMode(captureModeIndex) {
        captureMode.currentIndex = captureModeIndex;
    }

    RowLayout {
        id: topLayout

        ColumnLayout {
            id: leftColumn
            Layout.preferredWidth: 400;

            Item {
                Layout.preferredWidth: 384;
                Layout.preferredHeight: 256;

                Item {
                    id: screenshotContainer;
                    anchors.fill: parent;
                    visible: false;

                    Image {
                        id: screenshotImage;

                        width: parent.width - 10;
                        height: parent.height - 10;

                        anchors.centerIn: parent;

                        fillMode: Image.PreserveAspectFit;
                        smooth: true;

                        function refreshImage() {
                            var rstring = Math.random().toString().substring(4);
                            screenshotImage.source = "image://screenshot/" + rstring;
                        }
                    }
                }

                DropShadow {
                    anchors.fill: screenshotContainer;
                    source: screenshotContainer;

                    horizontalOffset: 0;
                    verticalOffset: 0;
                    radius: 5;
                    samples: 32;
                    color: "black";
                }
            }
        }

        ColumnLayout {
            id: rightColumn
            spacing: 20;

            Label {
                text: i18n("Capture Mode");

                font.bold: true;
                font.pointSize: 12;
            }

            ColumnLayout {
                id: innerColumnLayoutCaptureMode;

                RowLayout {
                    id: captureAreaLayout;
                    anchors.left: parent.left;
                    anchors.leftMargin: 32;

                    Label { text: i18n("Capture Area"); }
                    ComboBox {
                        id: captureMode;
                        model: captureModeModel;

                        onCurrentIndexChanged: {
                            saveCaptureMode(captureMode.currentIndex);

                            var capturemode = captureModeModel.get(captureMode.currentIndex)["type"];
                            if (capturemode === "fullScreen") {
                                optionMousePointer.enabled = true;
                                optionWindowDecorations.enabled = false;
                            } else if (capturemode === "currentScreen") {
                                optionMousePointer.enabled = true;
                                optionWindowDecorations.enabled = false;
                            } else if (capturemode === "activeWindow") {
                                optionMousePointer.enabled = true;
                                optionWindowDecorations.enabled = true;
                            } else if (capturemode === "rectangularRegion") {
                                optionMousePointer.enabled = false;
                                optionWindowDecorations.enabled = false;
                            }
                        }

                        Layout.preferredWidth: 200;
                    }
                }

                RowLayout {
                    id: captureDelayLayout;
                    anchors.right: captureAreaLayout.right;

                    Label { text: i18n("Capture Delay"); }
                    SpinBox {
                        id: captureDelay;

                        minimumValue: 0.0;
                        maximumValue: 999.9;
                        decimals: 1;
                        stepSize: 0.1;
                        suffix: i18n(" seconds");

                        Layout.preferredWidth: 200;
                    }
                }
            }

            Label {
                text: i18n("Capture Options");

                font.bold: true;
                font.pointSize: 12;
            }

            ColumnLayout {
                id: innerColumnLayoutCaptureOptions;
                spacing: 10;

                CheckBox {
                    id: optionMousePointer;
                    anchors.left: parent.left;
                    anchors.leftMargin: 32;

                    onCheckedChanged: saveCheckboxStates(optionMousePointer.checked, optionWindowDecorations.checked);

                    text: i18n("Include mouse pointer");
                }

                CheckBox {
                    id: optionWindowDecorations;
                    anchors.left: parent.left;
                    anchors.leftMargin: 32;

                    onCheckedChanged: saveCheckboxStates(optionMousePointer.checked, optionWindowDecorations.checked);

                    text: i18n("Include window titlebar and borders");
                }
            }
        }
    }

    RowLayout {
        id: bottomLayout;

        Layout.alignment: Qt.AlignHCenter | Qt.AlignTop;
        Layout.preferredHeight: 50;

        Button {
            id: takeNewScreenshot;
            text: i18n("Take New Screenshot");
            focus: true;

            onClicked: requestNew();

            function requestNew() {
                var capturemode = captureModeModel.get(captureMode.currentIndex)["type"];
                var capturedelay = captureDelay.value;
                var includepointer = optionMousePointer.checked;
                var includedecor = optionWindowDecorations.checked;

                newScreenshotRequest(capturemode, capturedelay, includepointer, includedecor);
            }
        }

        Button {
            id: editCurrentImage;
            text: i18n("Edit Image");

            onClicked: editScreenshotRequest();
        }
    }

    Rectangle {
        Layout.preferredHeight: 1;
        Layout.fillWidth: true;

        color: "darkgrey";
    }

    ListModel {
        id: captureModeModel
        dynamicRoles: true;

        Component.onCompleted: {
            captureModeModel.append({ type: "fullScreen", text: "Full Screen (All Monitors)" });
            captureModeModel.append({ type: "currentScreen", text: i18n("Current Screen") });
            captureModeModel.append({ type: "activeWindow", text: i18n("Active Window") });
            captureModeModel.append({ type: "rectangularRegion", text: i18n("Rectangular Region") });
        }
    }
}
