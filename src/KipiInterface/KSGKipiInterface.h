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

#ifndef KSGKIPIINTERFACE_H
#define KSGKIPIINTERFACE_H

#include <QList>
#include <QWidget>

#include <KIPI/Interface>
#include <KIPI/UploadWidget>
#include <KIPI/ImageCollection>
#include <KIPI/ImageCollectionSelector>
#include <KIPI/PluginLoader>
#include <KIPI/ImageInfo>

class KSGKipiInterface : public KIPI::Interface
{
    Q_OBJECT

    public:

    explicit KSGKipiInterface(QObject *parent = nullptr);
    ~KSGKipiInterface() Q_DECL_OVERRIDE;

    bool addImage(const QUrl &, QString &err) Q_DECL_OVERRIDE;
    void delImage(const QUrl &) Q_DECL_OVERRIDE;
    void refreshImages(const QList<QUrl> &urls) Q_DECL_OVERRIDE;

    KIPI::FileReadWriteLock *createReadWriteLock(const QUrl &url) const Q_DECL_OVERRIDE;
    KIPI::MetadataProcessor *createMetadataProcessor() const Q_DECL_OVERRIDE;

    KIPI::ImageCollection currentAlbum() Q_DECL_OVERRIDE;
    KIPI::ImageCollection currentSelection() Q_DECL_OVERRIDE;
    QList<KIPI::ImageCollection> allAlbums() Q_DECL_OVERRIDE;

    KIPI::ImageCollectionSelector *imageCollectionSelector(QWidget *parent) Q_DECL_OVERRIDE;
    KIPI::UploadWidget *uploadWidget(QWidget *parent) Q_DECL_OVERRIDE;

    int features() const Q_DECL_OVERRIDE;
    KIPI::ImageInfo info(const QUrl &) Q_DECL_OVERRIDE;

    private:

    QObject            *mScreenGenie;
    KIPI::PluginLoader *mPluginLoader;
};

#endif // KSGKIPIINTERFACE_H


