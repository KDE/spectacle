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

#include "KSGKipiImageCollectionSelector.h"
#include <KLocalizedString>
#include <QVBoxLayout>

KSGKipiImageCollectionSelector::KSGKipiImageCollectionSelector(KIPI::Interface *interface, QWidget *parent)
    : KIPI::ImageCollectionSelector(parent),
      mInterface(interface),
      mListWidget(new QListWidget)
{
    const auto allAlbums = interface->allAlbums();
    for (const auto &collection : allAlbums) {
        QListWidgetItem *item = new QListWidgetItem(mListWidget);
        QString name = collection.name();
        int imageCount = collection.images().size();
        QString title = i18ncp("%1 is collection name, %2 is image count in collection",
                               "%1 (%2 image)", "%1 (%2 images)", name, imageCount);
        item->setText(title);
        item->setData(Qt::UserRole, name);
    }
    connect(mListWidget, &QListWidget::currentRowChanged, this, &KIPI::ImageCollectionSelector::selectionChanged);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(mListWidget);
    layout->setContentsMargins(0, 0, 0, 0);
}

KSGKipiImageCollectionSelector::~KSGKipiImageCollectionSelector()
{}

QList<KIPI::ImageCollection> KSGKipiImageCollectionSelector::selectedImageCollections() const
{
    QListWidgetItem *item = mListWidget->currentItem();

    QList<KIPI::ImageCollection> selectedList;
    if (item) {
        QString name = item->data(Qt::UserRole).toString();
        const auto allAlbums = mInterface->allAlbums();
        for (const auto &collection : allAlbums) {
            if (collection.name() == name) {
                selectedList.append(collection);
                break;
            }
        }
    }
    return selectedList;
}


