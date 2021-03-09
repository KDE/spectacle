/*
 *  SPDX-FileCopyrightText: 2020 Méven Car <meven.car@enioka.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
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
        return x() < other.x() || ( x() == other.x() && y() < other.y() );
    }
};

#endif // COMPARABLEQPOINT_H
