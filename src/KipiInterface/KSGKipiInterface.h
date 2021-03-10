/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <me@BaloneyGeek.com>
 *  SPDX-FileCopyrightText: 2010 Pau Garcia i Quiles <pgquiles@elpauer.org>
 *  Essentially a rip-off of code for Kamoso by:
 *  SPDX-FileCopyrightText: 2008-2009 Aleix Pol <aleixpol@kde.org>
 *  SPDX-FileCopyrightText: 2008-2009 Alex Fiestas <alex@eyeos.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
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
    ~KSGKipiInterface() override;

    bool addImage(const QUrl &, QString &err) override;
    void delImage(const QUrl &) override;
    void refreshImages(const QList<QUrl> &urls) override;

    KIPI::FileReadWriteLock *createReadWriteLock(const QUrl &url) const override;
    KIPI::MetadataProcessor *createMetadataProcessor() const override;

    KIPI::ImageCollection currentAlbum() override;
    KIPI::ImageCollection currentSelection() override;
    QList<KIPI::ImageCollection> allAlbums() override;

    KIPI::ImageCollectionSelector *imageCollectionSelector(QWidget *parent) override;
    KIPI::UploadWidget *uploadWidget(QWidget *parent) override;

    int features() const override;
    KIPI::ImageInfo info(const QUrl &) override;

    private:

    QObject            *mScreenGenie;
    KIPI::PluginLoader *mPluginLoader;
};

#endif // KSGKIPIINTERFACE_H


