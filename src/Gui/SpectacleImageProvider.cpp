/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SpectacleImageProvider.h"
#include "SelectionEditor.h"

#include "ExportManager.h"

#include <QPixmapCache>
#include <qnamespace.h>

#include <cmath>

SpectacleImageProvider::SpectacleImageProvider(ImageType type, Flags flags)
    : QQuickImageProvider(type, flags)
{
}

QPixmap SpectacleImageProvider::requestPixmap(const QString &id, QSize *size, const QSize& requestedSize)
{
    // `id` is the "my_id" part of this example url: "image://spectacle/my_id"
    // `size` is the original image size.
    // `requestedSize` is `sourceSize` when used with QML Image items.

    QPixmap pixmap;
    auto parts = id.split(QLatin1Char('/'));
    if (parts.count() == 3 && parts.first() == QStringLiteral("screen")) {
        pixmap = QPixmap::fromImage(SelectionEditor::instance()->imageForScreenName(parts[1]));
    } else {
        pixmap = ExportManager::instance()->pixmap();
    }

    // `size` is a pointer to a QSize variable in `QQuickPixmapReader::processJob()`.
    if (size) {
        *size = pixmap.size();
    }

    // The requestedSize is already scaled by the devicePixelRatio
    // in `QQuickImageBase::loadPixmap()`.
    if (!pixmap.isNull() && !requestedSize.isEmpty() && size && *size != requestedSize) {
        QSize scaledSize = size->scaled(requestedSize.width(),
                                        requestedSize.height(),
                                        Qt::KeepAspectRatio);
        const int w = scaledSize.width();
        const int h = scaledSize.height();
        auto transformation = Qt::SmoothTransformation;
        if (std::fmod(w, size->width()) == 0.0) {
            transformation = Qt::FastTransformation;
        }
        if (w > 0 && h <= 0) {
            return pixmap.scaledToWidth(w, transformation);
        } else if (w <= 0 && h > 0) {
            return pixmap.scaledToHeight(h, transformation);
        } else if (w > 0 && h > 0) {
            return pixmap.scaled(w, h, Qt::KeepAspectRatio, transformation);
        }
    }
    return pixmap;
}
