/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <me@BaloneyGeek.com>
 *  SPDX-FileCopyrightText: 2010 Pau Garcia i Quiles <pgquiles@elpauer.org>
 *  based on code for Gwenview by
 *  SPDX-FileCopyrightText: 2008 Aurélien Gâteau <agateau@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
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


