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

function drawCropRectangle(ctx, x, y, w, h, sStyle, fStyle, lWidth) {
    // set up the colours
    ctx.fillStyle = sStyle;

    // draw the border and cut out the selection
    ctx.fillRect(x, y, w, h);
    ctx.clearRect(x + lWidth, y + lWidth, w - (lWidth * 2), h - (lWidth * 2));
};

function drawCropHandles(ctx, x, y, w, h, sStyle, fStyle, lWidth) {
    ctx.strokeStyle = sStyle;
    ctx.fillStyle = fStyle;

    if ((w > 20) && (h > 20)) {
        // top-left handle
        ctx.beginPath();
        ctx.arc(x, y, 8, 0, 0.5 * Math.PI);
        ctx.lineTo(x, y);
        ctx.fill();

        // top-right handle
        ctx.beginPath();
        ctx.arc(x + w, y, 8, 0.5 * Math.PI, Math.PI);
        ctx.lineTo(x + w, y);
        ctx.fill();

        // bottom-left handle
        ctx.beginPath();
        ctx.arc(x + w, y + h, 8, Math.PI, 1.5 * Math.PI);
        ctx.lineTo(x + w, y + h);
        ctx.fill();

        // bottom-right handle
        ctx.beginPath();
        ctx.arc(x, y + h, 8, 1.5 * Math.PI, 2 * Math.PI);
        ctx.lineTo(x, y + h);
        ctx.fill();

        // top-center handle
        ctx.beginPath();
        ctx.arc(x + w / 2, y, 5, 0, Math.PI);
        ctx.fill();

        // right-center handle
        ctx.beginPath();
        ctx.arc(x + w, y + h / 2, 5, 0.5 * Math.PI, 1.5 * Math.PI);
        ctx.fill();

        // bottom-center handle
        ctx.beginPath();
        ctx.arc(x + w / 2, y + h, 5, Math.PI, 2 * Math.PI);
        ctx.fill();

        // left-center handle
        ctx.beginPath();
        ctx.arc(x, y + h / 2, 5, 1.5 * Math.PI, 0.5 * Math.PI);
        ctx.fill();
    }
};
