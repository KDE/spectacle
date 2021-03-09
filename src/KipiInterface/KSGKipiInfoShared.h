/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <me@BaloneyGeek.com>
 *  SPDX-FileCopyrightText: 2010 Pau Garcia i Quiles <pgquiles@elpauer.org>
 *  Essentially a rip-off of code for Kamoso by:
 *  SPDX-FileCopyrightText: 2008-2009 Aleix Pol <aleixpol@kde.org>
 *  SPDX-FileCopyrightText: 2008-2009 Alex Fiestas <alex@eyeos.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSGKIPIINFOSHARED_H
#define KSGKIPIINFOSHARED_H


#include <KIPI/ImageInfoShared>
#include <KIPI/Interface>

class KSGKipiInfoShared : public KIPI::ImageInfoShared
{
    public:

    explicit KSGKipiInfoShared(KIPI::Interface *interface, const QUrl &url);
    ~KSGKipiInfoShared() override;

    void addAttributes(const QMap< QString, QVariant > &) override;
    void delAttributes(const QStringList &) override;
    void clearAttributes() override;
    QMap< QString, QVariant > attributes() override;
    virtual void setDescription(const QString &);
    virtual QString description();
};

#endif


