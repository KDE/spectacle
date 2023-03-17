/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QImage>
#include <QRectF>

struct CanvasImage
{
    CanvasImage(const QImage &image = {}, qreal devicePixelRatio = -1)
        : image(image)
    {
        if (devicePixelRatio > 0) {
            this->image.setDevicePixelRatio(devicePixelRatio);
        }
        setAutoSize(this->image.devicePixelRatio());
    }

    CanvasImage(const QImage &image, const QSizeF &size)
        : image(image)
        , rect({0, 0}, size)
    {
        setAutoDevicePixelRatio(rect);
        setAutoSize(this->image.devicePixelRatio());
    }

    CanvasImage(const QImage &image,
                const QRectF &rect)
        : image(image)
        , rect(rect)
    {
        setAutoDevicePixelRatio(rect);
        setAutoSize(this->image.devicePixelRatio());
    }

    void setAutoDevicePixelRatio(const QRectF &rect) {
        const QSizeF &imageSize = image.size();
        qreal dpr = image.devicePixelRatio();
        if (dpr == 1 && !imageSize.isEmpty() && !rect.isEmpty() && imageSize != rect.size()) {
            dpr = imageSize.width() / rect.width();
            image.setDevicePixelRatio(dpr);
        }
    }

    void setAutoSize(qreal devicePixelRatio) {
        const QSizeF &imageSize = image.size();
        if (!imageSize.isEmpty() && rect.isEmpty()) {
            rect.setSize(imageSize / devicePixelRatio);
        }
    }

    /**
     * Holds the QImage.
     *
     * Be sure to set the devicePixelRatio of the QImage if not set via the constructor.
     */
    QImage image;

    /**
     * Holds the QRectF that should be used for rendering the image in the GUI.
     *
     * This should be scaled based on the devicePixelRatio.
     * Use image.size() if you want to get the true image size.
     */
    QRectF rect;
};
