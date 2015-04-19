/*
 *  Copyright (C) 2015 Boudhayan Gupta <me@BaloneyGeek.com>
 *  Copyright (C) 2010 Pau Garcia i Quiles <pgquiles@elpauer.org>
 *  Essentially a rip-off of code for Kamoso by:
 *  Copyright (C) 2008-2009 by Aleix Pol <aleixpol@kde.org>
 *  Copyright (C) 2008-2009 by Alex Fiestas <alex@eyeos.org>
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

#ifndef KSGKIPIIMAGECOLLECTIONSHARED_H
#define KSGKIPIIMAGECOLLECTIONSHARED_H

#include <QObject>
#include <QUrl>

#include <KIPI/ImageCollectionShared>

class KSGKipiImageCollectionShared : public KIPI::ImageCollectionShared
{
    public:

    explicit KSGKipiImageCollectionShared(QObject *ksg);
    ~KSGKipiImageCollectionShared();

    QString name();
    QString comment();
    QList<QUrl> images();
    QUrl uploadRoot();
    QString uploadRootName();
    bool isDirectory();

    private:

    QList<QUrl> mImages;
    QObject *mScreenGenie;
};

#endif

