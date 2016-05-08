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
