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

import "Shapes/ShapeContainer.Rectangle.js" as RectContainer
import "Shapes/Shape.CropRectangle.js" as CropRectangle

Item {
    id: cropSurface;
    objectName: "cropSurface";

    // interaction active
    property bool interactionActive: false;

    // the selection
    property var mSelection: null;
    property var mCurrentHandle: null;
    property bool mDragging: false;
    property bool mDrawing: false;
    property color maskColour: Qt.rgba(0, 0, 0, 0.75);
    property color strokeColour: Qt.rgba(0.114, 0.6, 0.953, 1);

    // selection getters
    function getSelectionX() { if (mSelection) { return mSelection.x; } else { return -1; } }
    function getSelectionY() { if (mSelection) { return mSelection.y; } else { return -1; } }
    function getSelectionW() { if (mSelection) { return mSelection.w; } else { return -1; } }
    function getSelectionH() { if (mSelection) { return mSelection.h; } else { return -1; } }

    // signal to accept selection
    signal acceptImage();

    // convenience functions
    function setInitialSelection(xx, yy, hh, ww) {
        mSelection = new RectContainer.ShapeContainer(xx, yy, hh, ww, strokeColour, maskColour, 1,
                                                      CropRectangle.drawCropRectangle,
                                                      CropRectangle.drawCropHandles, strokeColour);
        cropSurfaceCanvas.requestPaint();
    }

    // the draw canvas
    Canvas {
        id: cropSurfaceCanvas;
        objectName: "cropSurfaceCanvas";
        anchors.fill: parent;

        renderTarget: Canvas.FramebufferObject;
        renderStrategy: Canvas.Cooperative;

        onPaint: {
            // get a context on the canvas and clear it
            var ctx = cropSurfaceCanvas.getContext("2d");
            ctx.clearRect(0, 0, width, height);

            // draw a sheet all over the canvas
            ctx.strokeStyle = strokeColour;
            ctx.fillStyle = maskColour;
            ctx.fillRect(0, 0, width, height);

            // if there's a selection, draw the cutout and handles
            if (mSelection) {
                mSelection.drawShape(ctx);
                mSelection.drawHandles(ctx);
            }
        }
    }

    // the mouse area to interact with the cropper
    MouseArea {
        id: cropSurfaceInteractionArea;
        objectName: "cropSurfaceInteractionArea";
        enabled: interactionActive;

        anchors.fill: parent;
        cursorShape: Qt.CrossCursor;
        hoverEnabled: true;

        property int dx: 0;
        property int dy: 0;

        onPressed: {
            if (mSelection) {
                var curHandle = mSelection.withinHandleBounds(mouse.x, mouse.y);
                if (curHandle >= 0) {
                    mCurrentHandle = curHandle;
                    cursorShape = Qt.ClosedHandCursor;
                } else if (mSelection.withinShapeBounds(mouse.x, mouse.y)) {
                    dx = mouse.x - mSelection.x;
                    dy = mouse.y - mSelection.y;
                    cursorShape = Qt.ClosedHandCursor;
                    mDragging = true;
                } else {
                    dx = mouse.x;
                    dy = mouse.y;
                    mSelection = new RectContainer.ShapeContainer(dx, dy, 1, 1, strokeColour, maskColour, 1,
                                                                  CropRectangle.drawCropRectangle,
                                                                  CropRectangle.drawCropHandles, strokeColour);
                    mDrawing = true;
                    cursorShape = Qt.CrossCursor;
                }
            } else {
                dx = mouse.x;
                dy = mouse.y;
                mSelection = new RectContainer.ShapeContainer(dx, dy, 1, 1, strokeColour, maskColour, 1,
                                                              CropRectangle.drawCropRectangle,
                                                              CropRectangle.drawCropHandles, strokeColour);
                mDrawing = true;
                cursorShape = Qt.CrossCursor;
            }
        }

        onPositionChanged: {
            if (pressed) {
                if (mSelection) {
                    if (mDragging) {
                        var xmax = width - mSelection.w;
                        var ymax = height - mSelection.h;
                        var xset = mouse.x - dx;
                        var yset = mouse.y - dy;

                        if ((xset >= 0) && (xset <= xmax) && (yset >= 0) && (yset <= ymax)) {
                            mSelection.x = xset;
                            mSelection.y = yset;
                        }
                    } else if (mCurrentHandle != null) {
                        mSelection.moveHandle(mCurrentHandle, mouse.x, mouse.y);
                    } else if (mDrawing) {
                        var tx = Math.min(mouse.x, dx);
                        var ty = Math.min(mouse.y, dy);
                        var tw = Math.max(mouse.x, dx) - tx;
                        var th = Math.max(mouse.y, dy) - ty;

                        mSelection = new RectContainer.ShapeContainer(tx, ty, tw, th, strokeColour, maskColour, 1,
                                                                      CropRectangle.drawCropRectangle,
                                                                      CropRectangle.drawCropHandles, strokeColour);
                    }
                }
                cropSurfaceCanvas.requestPaint();
            } else {
                if (mSelection && (mSelection.withinShapeBounds(mouse.x, mouse.y)
                    || (mSelection.withinHandleBounds(mouse.x, mouse.y) >= 0))) {
                    cursorShape = Qt.OpenHandCursor;
                } else {
                    cursorShape = Qt.CrossCursor;
                }
            }
        }

        onReleased: {
            cursorShape = Qt.OpenHandCursor;

            mCurrentHandle = null;
            mDragging = false;
            mDrawing = false;
            dx = 0;
            dy = 0;

            cropSurfaceCanvas.requestPaint();
        }

        onDoubleClicked: {
            acceptImage();
        }
    }
}
