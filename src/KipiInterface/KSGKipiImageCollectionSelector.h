/*
 *  Copyright (C) 2015 Boudhayan Gupta <me@BaloneyGeek.com>
 *  Copyright 2010 Pau Garcia i Quiles <pgquiles@elpauer.org>
 *  based on code for Gwenview by
 *  Copyright 2008 Aurélien Gâteau <agateau@kde.org>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.
 *
*/

#ifndef KSGKIPIIMAGECOLLECTIONSELECTOR_H
#define KSGKIPIIMAGECOLLECTIONSELECTOR_H

#include <QListWidget>
#include <QVBoxLayout>

#include <KLocalizedString>

#include <KIPI/Interface>
#include <KIPI/ImageCollection>
#include <KIPI/ImageCollectionSelector>

class KSGKipiImageCollectionSelector : public KIPI::ImageCollectionSelector
{
    Q_OBJECT

    public:

    explicit KSGKipiImageCollectionSelector(KIPI::Interface *interface, QWidget *parent);
    ~KSGKipiImageCollectionSelector();

    QList<KIPI::ImageCollection> selectedImageCollections() const;

    private:

    KIPI::Interface *mInterface;
    QListWidget     *mListWidget;
};

#endif // KSGKIPIIMAGECOLLECTIONSELECTOR_H


