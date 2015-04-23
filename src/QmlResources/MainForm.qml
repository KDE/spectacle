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

import QtQuick 2.0
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import QtGraphicalEffects 1.0

ColumnLayout {
    id: mainLayout;
    spacing: 10;

    signal newScreenshotRequest(string captureType, real captureDelay, bool includePointer, bool includeDecorations);
    signal saveCheckboxStates(bool includePointer, bool includeDecorations, bool waitCaptureOnClick);
    signal saveCaptureMode(int captureModeIndex);
    signal startDragAndDrop;

    function saveCheckboxes() {
        saveCheckboxStates(optionMousePointer.checked, optionWindowDecorations.checked, captureOnClick.checked);
    }

    function reloadScreenshot() {
        screenshotImage.refreshImage();
    }

    function loadCheckboxStates(includePointer, includeDecorations, waitCaptureOnClick) {
        optionMousePointer.checked = includePointer;
        optionWindowDecorations.checked = includeDecorations;
        captureOnClick.checked = waitCaptureOnClick;
    }

    function loadCaptureMode(captureModeIndex) {
        captureMode.currentIndex = captureModeIndex;
    }

    function disableOnClick() {
        captureOnClick.checked = false;
        captureOnClick.enabled = false;
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

                MouseArea {
                    objectName: "screenshotDragMouseArea";

                    anchors.fill: screenshotContainer;
                    cursorShape: Qt.OpenHandCursor;

                    property int oldMouseX;
                    property int oldMouseY;
                    property bool dragEmitted: false;

                    onPressed: {
                        oldMouseX = mouseX;
                        oldMouseY = mouseY;
                        dragEmitted = false;

                        cursorShape = Qt.ClosedHandCursor;
                    }

                    onReleased: {
                        dragEmitted = false;
                        cursorShape = Qt.OpenHandCursor;

                    }

                    onPositionChanged: {
                        if ( mouseX < (oldMouseX - 5) || mouseX > (oldMouseX + 5) ||
                             mouseY < (oldMouseY - 5) || mouseY > (oldMouseY + 5) ) {

                            if (!dragEmitted) {
                                startDragAndDrop();
                                dragEmitted = true;
                            }
                        }
                    }
                }
            }
        }

        ColumnLayout {
            id: rightColumn
            spacing: 20;

            Layout.preferredWidth: 400;

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

                    Label {
                        id: captureAreaLabel;
                        text: i18n("Capture Area");
                    }

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

                    Label {
                        id: captureDelayLabel;
                        text: i18n("Capture Delay");
                    }

                    SpinBox {
                        id: captureDelay;

                        minimumValue: -0.1;
                        maximumValue: 999.9;
                        decimals: 1;
                        stepSize: 0.1;
                        suffix: i18n(" seconds");

                        Layout.preferredWidth: 120;
                    }

                    CheckBox {
                        id: captureOnClick;

                        checked: false;
                        text: i18n("On Click");

                        onCheckedChanged: {
                            if (checked) {
                                captureDelay.enabled = false;
                            } else {
                                captureDelay.enabled = true;
                            }

                            mainLayout.saveCheckboxes();
                        }

                        Layout.preferredWidth: 75;
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

                    onCheckedChanged: mainLayout.saveCheckboxes();

                    text: i18n("Include mouse pointer");
                }

                CheckBox {
                    id: optionWindowDecorations;
                    anchors.left: parent.left;
                    anchors.leftMargin: 32;

                    onCheckedChanged: mainLayout.saveCheckboxes();

                    text: i18n("Include window titlebar and borders");
                }
            }

            Button {
                id: takeNewScreenshot;
                text: i18n("Take New Screenshot");
                iconName: "ksnapshot"
                focus: true;

                Layout.alignment: Qt.AlignHCenter | Qt.AlignTop;

                onClicked: {
                    var capturemode = captureModeModel.get(captureMode.currentIndex)["type"];
                    var capturedelay = captureOnClick.checked ? -1 : captureDelay.value;
                    var includepointer = optionMousePointer.checked;
                    var includedecor = optionWindowDecorations.checked;

                    newScreenshotRequest(capturemode, capturedelay, includepointer, includedecor);
                }
            }
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
