/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <me@BaloneyGeek.com>
 *  SPDX-FileCopyrightText: 2010 Pau Garcia i Quiles <pgquiles@elpauer.org>
 *  Essentially a rip-off of code for Kamoso by:
 *  SPDX-FileCopyrightText: 2008-2009 Aleix Pol <aleixpol@kde.org>
 *  SPDX-FileCopyrightText: 2008-2009 Alex Fiestas <alex@eyeos.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KSGKIPIIMAGECOLLECTIONSHARED_H
#define KSGKIPIIMAGECOLLECTIONSHARED_H

#include <QUrl>

#include <KIPI/ImageCollectionShared>

class KSGKipiImageCollectionShared : public KIPI::ImageCollectionShared
{
public:
    explicit KSGKipiImageCollectionShared();
    ~KSGKipiImageCollectionShared() override;

    QString name() override;
    QString comment() override;
    QList<QUrl> images() override;
    virtual QUrl uploadRoot();
    QString uploadRootName() override;
    bool isDirectory() override;

private:
    QList<QUrl> mImages;
};

#endif
