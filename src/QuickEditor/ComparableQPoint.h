/*
 *  Copyright (C) 2020 Méven Car <meven.car@enioka.com>
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

#include <QPoint>

#ifndef COMPARABLEQPOINT_H
#define COMPARABLEQPOINT_H


class ComparableQPoint : public QPoint
{
public:
    ComparableQPoint(const QPoint &point): QPoint(point.x(), point.y())
    {}

    ComparableQPoint(): QPoint()
    {}

    // utility class that allows using QMap to sort its keys when they are QPoint
    bool operator<(const ComparableQPoint &other) const {
        return x() < other.x() || y() < other.y();
    }
};

#endif // COMPARABLEQPOINT_H
