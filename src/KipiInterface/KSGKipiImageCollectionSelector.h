/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <me@BaloneyGeek.com>
 *  SPDX-FileCopyrightText: 2010 Pau Garcia i Quiles <pgquiles@elpauer.org>
 *  based on code for Gwenview by
 *  SPDX-FileCopyrightText: 2008 Aurélien Gâteau <agateau@kde.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#ifndef KSGKIPIIMAGECOLLECTIONSELECTOR_H
#define KSGKIPIIMAGECOLLECTIONSELECTOR_H

#include <QListWidget>

#include <KIPI/ImageCollection>
#include <KIPI/ImageCollectionSelector>
#include <KIPI/Interface>

class KSGKipiImageCollectionSelector : public KIPI::ImageCollectionSelector
{
    Q_OBJECT

public:
    explicit KSGKipiImageCollectionSelector(KIPI::Interface *interface, QWidget *parent);
    ~KSGKipiImageCollectionSelector() override;

    QList<KIPI::ImageCollection> selectedImageCollections() const override;

private:
    KIPI::Interface *const mInterface;
    QListWidget *const mListWidget;
};

#endif // KSGKIPIIMAGECOLLECTIONSELECTOR_H
