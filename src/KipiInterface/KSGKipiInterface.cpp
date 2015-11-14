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

#include "KSGKipiInterface.h"
#include "KSGKipiInfoShared.h"
#include "KSGKipiImageCollectionShared.h"
#include "KSGKipiImageCollectionSelector.h"

KSGKipiInterface::KSGKipiInterface(QObject *parent)
    : KIPI::Interface(parent)
{}

KSGKipiInterface::~KSGKipiInterface()
{}

// no-op single image handlers

bool KSGKipiInterface::addImage(const QUrl &, QString &)  { return true; }
void KSGKipiInterface::delImage(const QUrl &)             {}
void KSGKipiInterface::refreshImages(const QList<QUrl> &) {}

// album handlers. mostly no-op

KIPI::ImageCollection KSGKipiInterface::currentAlbum()
{
    return KIPI::ImageCollection(new KSGKipiImageCollectionShared);
}

KIPI::ImageCollection KSGKipiInterface::currentSelection()
{
    return currentAlbum();
}

QList<KIPI::ImageCollection> KSGKipiInterface::allAlbums()
{
    return QList<KIPI::ImageCollection>({ currentAlbum() });
}

// features and info

KIPI::ImageInfo KSGKipiInterface::info(const QUrl &url)
{
    return KIPI::ImageInfo(new KSGKipiInfoShared(this, url));
}

int KSGKipiInterface::features() const
{
    return KIPI::ImagesHasTime;
}

// widgets and selectors

KIPI::ImageCollectionSelector *KSGKipiInterface::imageCollectionSelector(QWidget *parent)
{
    return new KSGKipiImageCollectionSelector(this, parent);
}

KIPI::UploadWidget *KSGKipiInterface::uploadWidget(QWidget *parent)
{
    return new KIPI::UploadWidget(parent);
}

// deal with api breakage

KIPI::FileReadWriteLock *KSGKipiInterface::createReadWriteLock(const QUrl &url) const
{
    Q_UNUSED(url);
    return NULL;
}

KIPI::RawProcessor *KSGKipiInterface::createRawProcessor() const
{
    return NULL;
}

KIPI::MetadataProcessor *KSGKipiInterface::createMetadataProcessor() const
{
    return NULL;
}
