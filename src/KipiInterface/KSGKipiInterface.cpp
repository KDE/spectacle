/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <me@BaloneyGeek.com>
 *  SPDX-FileCopyrightText: 2010 Pau Garcia i Quiles <pgquiles@elpauer.org>
 *  Essentially a rip-off of code for Kamoso by:
 *  SPDX-FileCopyrightText: 2008-2009 Aleix Pol <aleixpol@kde.org>
 *  SPDX-FileCopyrightText: 2008-2009 Alex Fiestas <alex@eyeos.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KSGKipiInterface.h"
#include "KSGKipiImageCollectionSelector.h"
#include "KSGKipiImageCollectionShared.h"
#include "KSGKipiInfoShared.h"

KSGKipiInterface::KSGKipiInterface(QObject *parent)
    : KIPI::Interface(parent)
{
}

KSGKipiInterface::~KSGKipiInterface()
{
}

// no-op single image handlers

bool KSGKipiInterface::addImage(const QUrl &, QString &)
{
    return true;
}
void KSGKipiInterface::delImage(const QUrl &)
{
}
void KSGKipiInterface::refreshImages(const QList<QUrl> &)
{
}

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
    return QList<KIPI::ImageCollection>({currentAlbum()});
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
    return nullptr;
}

KIPI::MetadataProcessor *KSGKipiInterface::createMetadataProcessor() const
{
    return nullptr;
}
