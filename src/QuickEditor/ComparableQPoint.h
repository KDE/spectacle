/*
 *  SPDX-FileCopyrightText: 2020 Méven Car <meven.car@enioka.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef COMPARABLEQPOINT_H
#define COMPARABLEQPOINT_H

#include <QPoint>

class ComparableQPoint : public QPoint
{
public:
    ComparableQPoint(QPoint point): QPoint(point)
    {}

    ComparableQPoint() = default;

    // utility class that allows using QMap to sort its keys when they are QPoint
    bool operator<(ComparableQPoint other) const {
        return x() < other.x() || ( x() == other.x() && y() < other.y() );
    }
};

#endif // COMPARABLEQPOINT_H
