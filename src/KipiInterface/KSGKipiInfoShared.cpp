/*
 *  SPDX-FileCopyrightText: 2010 Pau Garcia i Quiles <pgquiles@elpauer.org>
 *  Essentially a rip-off of code for Kamoso by:
 *  SPDX-FileCopyrightText: 2008-2009 Aleix Pol <aleixpol@kde.org>
 *  SPDX-FileCopyrightText: 2008-2009 Alex Fiestas <alex@eyeos.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KSGKipiInfoShared.h"
#include <KLocalizedString>

KSGKipiInfoShared::KSGKipiInfoShared(KIPI::Interface *interface, const QUrl &url)
    : KIPI::ImageInfoShared(interface, url)
{
}

KSGKipiInfoShared::~KSGKipiInfoShared()
{
}

// no-op functions

void KSGKipiInfoShared::delAttributes(const QStringList &)
{
}
void KSGKipiInfoShared::addAttributes(const QMap<QString, QVariant> &)
{
}
void KSGKipiInfoShared::clearAttributes()
{
}
QMap<QString, QVariant> KSGKipiInfoShared::attributes()
{
    return QMap<QString, QVariant>();
}
void KSGKipiInfoShared::setDescription(const QString &)
{
}
QString KSGKipiInfoShared::description()
{
    return i18n("Taken with Spectacle");
}
