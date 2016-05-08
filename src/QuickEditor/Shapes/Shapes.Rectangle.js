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

function Shape(x, y, w, h, sStyle, fStyle, lWidth) {
    // save the start and end coordinates
    this.x = x || 0;
    this.y = y || 0;
    this.w = w || 0;
    this.h = h || 0;

    // save the stroke and fill colours, line width
    this.sStyle = sStyle || "black";
    this.fStyle = fStyle || "black";
    this.lWidth = lWidth || 1;

    // draw the shape
    this.drawShape = function(ctx) {
        ctx.save();

        ctx.fillStyle = this.fStyle;
        ctx.strokeStyle = this.sStyle;
        ctx.lineWidth = this.lWidth;
        ctx.strokeRect(this.x, this.y, this.w, this.h);
        ctx.fillRect(this.x, this.y, this.w, this.h);

        ctx.restore();
    }

    // draw handles
    this.drawHandles = function(ctx) {
        ctx.save();

        // draw the border first
        ctx.strokeStyle = Qt.rgba(0.188, 0.368, 0.431, 1);
        ctx.lineWidth = 2;
        ctx.strokeRect(this.x, this.y, this.w, this.h);

        // draw the handles
        ctx.fillStyle = Qt.rgba(0.188, 0.368, 0.431, 1);

        // top left handle
        ctx.beginPath();
        ctx.arc(this.x, this.y, 10, 0, 2 * Math.PI);
        ctx.closePath();
        ctx.fill();

        // top middle handle
        ctx.beginPath();
        ctx.arc(this.x + (this.w / 2), this.y, 10, 0, 2 * Math.PI);
        ctx.closePath();
        ctx.fill();

        // top right handle
        ctx.beginPath();
        ctx.arc(this.x + this.w, this.y, 10, 0, 2 * Math.PI);
        ctx.closePath();
        ctx.fill();

        // middle left handle
        ctx.beginPath();
        ctx.arc(this.x, this.y + (this.h / 2), 10, 0, 2 * Math.PI);
        ctx.closePath();
        ctx.fill();

        // middle right handle
        ctx.beginPath();
        ctx.arc(this.x + this.w, this.y + (this.h / 2), 10, 0, 2 * Math.PI);
        ctx.closePath();
        ctx.fill();

        // bottom left handle
        ctx.beginPath();
        ctx.arc(this.x, this.y + this.h, 10, 0, 2 * Math.PI);
        ctx.closePath();
        ctx.fill();

        // bottom middle handle
        ctx.beginPath();
        ctx.arc(this.x + (this.w / 2), this.y + this.h, 10, 0, 2 * Math.PI);
        ctx.closePath();
        ctx.fill();

        // bottom right handle
        ctx.beginPath();
        ctx.arc(this.x + this.w, this.y + this.h, 10, 0, 2 * Math.PI);
        ctx.closePath();
        ctx.fill();

        // done
        ctx.restore();
    }

    // helper function to check if point is in bounds
    this._withinBounds = function(xx, yy, tx, ty, bx, by) {
        return ((xx >= tx) && (xx <= bx) && (yy >= ty) && (yy <= by));
    }

    // check if the mouse position is within the shape's bounds
    this.withinShapeBounds = function(xx, yy) {
        return this._withinBounds(xx, yy, this.x, this.y, this.x + this.w, this.y + this.h);
    }

    // check if the mouse position is within handle bounds
    this.withinHandleBounds = function(xx, yy) {
        // top left
        if (this._withinBounds(xx, yy, this.x - 10, this.y - 10,
                               this.x + 10, this.y + 10)) {
            return 0;
        }

        // top center
        if (this._withinBounds(xx, yy, (this.x + (this.w / 2)) - 10, this.y - 10,
                               (this.x + (this.w / 2)) + 10, this.y + 10)) {
            return 1;
        }

        // top right
        if (this._withinBounds(xx, yy, this.x + this.w - 10, this.y - 10,
                               this.x + this.w + 10, this.y + 10)) {
            return 2;
        }

        // middle left
        if (this._withinBounds(xx, yy, this.x - 10, (this.y + (this.h / 2)) - 10,
                               this.x + 10, (this.y + (this.h / 2)) + 10)) {
            return 3;
        }

        // middle right
        if (this._withinBounds(xx, yy, this.x + this.w - 10, (this.y + (this.h / 2)) - 10,
                               this.x + this.w + 10, (this.y + (this.h / 2)) + 10)) {
            return 4;
        }

        // bottom left
        if (this._withinBounds(xx, yy, this.x - 10, this.y + this.h - 10,
                               this.x + 10, this.y + this.h + 10)) {
            return 5;
        }

        // bottom center
        if (this._withinBounds(xx, yy, (this.x + (this.w / 2)) - 10, this.y + this.h - 10,
                               (this.x + (this.w / 2)) + 10, this.y + this.h + 10)) {
            return 6;
        }

        // bottom right
        if (this._withinBounds(xx, yy, this.x + this.w - 10, this.y + this.h - 10,
                               this.x + this.w + 10, this.y + this.h + 10)) {
            return 7;
        }

        // nothing
        return -1;
    }

    // move handles around
    this.moveHandle = function(handle, xx, yy) {
        // top left
        if (handle == 0) {
            if (xx < (this.x + this.w)) {
                this.w = this.w + (this.x - xx);
                this.x = xx;
            }

            if (yy < (this.y + this.h)) {
                this.h = this.h + (this.y - yy);
                this.y = yy;
            }

            return true;
        }

        // top center
        if (handle == 1) {
            if (yy < (this.y + this.h)) {
                this.h = this.h + (this.y - yy);
                this.y = yy;
            }

            return true;
        }

        // top right
        if (handle == 2) {
            if (xx > this.x) {
                this.w = xx - this.x;
            }

            if (yy < (this.y + this.h)) {
                this.h = this.h + (this.y - yy);
                this.y = yy;
            }

            return true;
        }

        // middle left
        if (handle == 3) {
            if (xx < (this.x + this.w)) {
                this.w = this.w + (this.x - xx);
                this.x = xx;
            }

            return true;
        }

        // middle right
        if (handle == 4) {
            if (xx > this.x) {
                this.w = xx - this.x;
            }

            return true;
        }

        // bottom left
        if (handle == 5) {
            if (xx < (this.x + this.w)) {
                this.w = this.w + (this.x - xx);
                this.x = xx;
            }

            if (yy > this.y) {
                this.h = yy - this.y;
            }

            return true;
        }

        // bottom center
        if (handle == 6) {
            if (yy > this.y) {
                this.h = yy - this.y;
            }

            return true;
        }

        // bottom right
        if (handle == 7) {
            if (xx > this.x) {
                this.w = xx - this.x;
            }

            if (yy > this.y) {
                this.h = yy - this.y;
            }

            return true;
        }

        // invalid handle
        return false;
    }
}
