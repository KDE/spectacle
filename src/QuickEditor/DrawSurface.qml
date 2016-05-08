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
import "Shapes/Shapes.Rectangle.js" as SRectangle

Item {
    id: drawSurface;
    objectName: "drawSurface";

    // scene manager draw queue
    property var mDrawQueue: [];

    // selections and handle
    property var mCurrentSelection: null;
    property var mCurrentHandle: null;
    property var mCurrentDrawing: null;

    // tool settings
    property var mCurrentTool: null;
    property color mStrokeColor: Qt.rgba(0, 0, 0, 1);
    property color mFillColor: Qt.rgba(1, 0, 0, 1);
    property int mLineWidth: 1;

    // access to the canvas
    property var drawCanvas: drawSurfaceCanvas;

    // convenience functions
    function deleteSelectedShape() {
        if (mCurrentSelection != null) {
            mDrawQueue.splice(mCurrentSelection, 1);
            mCurrentSelection = null;
            drawSurfaceCanvas.requestPaint();
        }
    }

    Component.onCompleted: {
        mCurrentTool = SRectangle;
        //mDrawQueue.push(new SRectangle.Shape(20, 20, 150, 150, "black", "blue"));
        //mDrawQueue.push(new SRectangle.Shape(600, 500, 150, 150, "black", "blue"));
    }

    // the draw canvas
    Canvas {
        id: drawSurfaceCanvas;
        objectName: "drawSurfaceCanvas";
        anchors.fill: parent;

        renderTarget: Canvas.FramebufferObject;
        renderStrategy: Canvas.Cooperative;

        onPaint: {
            var ctx = drawSurfaceCanvas.getContext("2d");
            ctx.clearRect(0, 0, width, height);

            for (var i = 0; i < mDrawQueue.length; i++) {
                mDrawQueue[i].drawShape(ctx);
            }

            if (mCurrentDrawing != null) {
                mCurrentDrawing.drawShape(ctx);
            }

            if (mCurrentSelection != null) {
                mDrawQueue[mCurrentSelection].drawHandles(ctx);
            }
        }
    }

    // the mouse area to interact with the shapes
    MouseArea {
        id: drawSurfaceInteractionArea;
        objectName: "drawSurfaceInteractionArea";

        anchors.fill: parent;
        cursorShape: Qt.CrossCursor;

        property int dx: 0;
        property int dy: 0;

        onClicked: {
            if (!mouse.wasHeld) {
                for (var i = mDrawQueue.length - 1; i >= 0; i--) {
                    if (mDrawQueue[i].withinShapeBounds(mouse.x, mouse.y)) {
                        mCurrentSelection = i;
                        drawSurfaceCanvas.requestPaint();
                        return;
                    }
                }
                mCurrentSelection = null;
                drawSurfaceCanvas.requestPaint();
            }
        }

        onPressed: {
            if (mCurrentSelection != null) {
                cursorShape = Qt.ClosedHandCursor;

                var curHandle = mDrawQueue[mCurrentSelection].withinHandleBounds(mouse.x, mouse.y);
                if (curHandle >= 0) {
                    mCurrentHandle = curHandle
                } else {
                    dx = mouse.x - mDrawQueue[mCurrentSelection].x;
                    dy = mouse.y - mDrawQueue[mCurrentSelection].y;
                }
            } else {
                dx = mouse.x;
                dy = mouse.y;
                mCurrentDrawing = new mCurrentTool.Shape(dx, dy, 1, 1, mStrokeColor, mFillColor);
            }
        }

        onPositionChanged: {
            if (mCurrentSelection != null) {
                if (mCurrentHandle != null) {
                    mDrawQueue[mCurrentSelection].moveHandle(mCurrentHandle, mouse.x, mouse.y);
                } else {
                    mDrawQueue[mCurrentSelection].x = mouse.x - dx;
                    mDrawQueue[mCurrentSelection].y = mouse.y - dy;
                }
            } else if (mCurrentDrawing != null) {
                var tx = Math.min(mouse.x, dx);
                var ty = Math.min(mouse.y, dy);
                var tw = Math.max(mouse.x, dx) - tx;
                var th = Math.max(mouse.y, dy) - ty;
                mCurrentDrawing = new mCurrentTool.Shape(tx, ty, tw, th, mStrokeColor, mFillColor);
            }
            drawSurfaceCanvas.requestPaint();
        }

        onReleased: {
            cursorShape = Qt.OpenHandCursor;

            mCurrentHandle = null;
            dx = 0;
            dy = 0;

            if (mCurrentDrawing != null) {
                mDrawQueue.push(mCurrentDrawing);
                mCurrentDrawing = null;
            }
            drawSurfaceCanvas.requestPaint();
        }
    }
}
