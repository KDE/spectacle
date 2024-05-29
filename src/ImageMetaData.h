/* SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QImage>
#include <QMap>
#include <QString>

namespace ImageMetaData
{
using namespace Qt::StringLiterals;

namespace Keys
{
static const auto windowTitle = u"windowTitle"_s;
static const auto screen = u"screen"_s;
// Replacement for QImage::offset since that only accepts QPoints
static const QString logicalX = u"logicalX"_s;
static const QString logicalY = u"logicalY"_s;
}

inline QString windowTitle(const QImage &image)
{
    return image.text(Keys::windowTitle);
}

inline void setWindowTitle(QImage &image, const QString &windowTitle)
{
    image.setText(Keys::windowTitle, windowTitle);
}

template<typename Map>
inline QString windowTitle(const Map &map)
{
    return map.value(Keys::windowTitle);
}

template<typename Map>
inline void setWindowTitle(Map &map, const QString &windowTitle)
{
    map[Keys::windowTitle] = windowTitle;
}

inline QString screen(const QImage &image)
{
    return image.text(Keys::screen);
}

inline void setScreen(QImage &image, const QString &screen)
{
    image.setText(Keys::screen, screen);
}

template<typename Map>
inline QString screen(const Map &map)
{
    return map.value(Keys::screen);
}

template<typename Map>
inline void setScreen(Map &map, const QString &screen)
{
    map[Keys::screen] = screen;
}

inline static void setLogicalXY(QImage &image, qreal x, qreal y)
{
    image.setText(Keys::logicalX, QString::number(x));
    image.setText(Keys::logicalY, QString::number(y));
}

inline static QPointF logicalXY(const QImage &image)
{
    return {image.text(Keys::logicalX).toDouble(), image.text(Keys::logicalY).toDouble()};
}

inline static void copy(QImage &target, const QImage &source)
{
    target.setDotsPerMeterX(source.dotsPerMeterX());
    target.setDotsPerMeterY(source.dotsPerMeterY());
    target.setDevicePixelRatio(source.devicePixelRatio());
    const auto keys = source.textKeys();
    for (const auto &key : keys) {
        target.setText(key, source.text(key));
    }
}
}
