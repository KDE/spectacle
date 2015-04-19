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

#include "KSGKipiImageCollectionShared.h"

KSGKipiImageCollectionShared::KSGKipiImageCollectionShared()  {}
KSGKipiImageCollectionShared::~KSGKipiImageCollectionShared() {}

QString KSGKipiImageCollectionShared::name()           { return "KScreenGenie"; }
QString KSGKipiImageCollectionShared::comment()        { return QString(); }
QString KSGKipiImageCollectionShared::uploadRootName() { return "/"; }
QUrl    KSGKipiImageCollectionShared::uploadRoot()     { return QUrl(uploadRootName()); }
bool    KSGKipiImageCollectionShared::isDirectory()    { return false; }

QList<QUrl> KSGKipiImageCollectionShared::images()
{
    QDir tempDir = QDir::temp();
    QString tempFile = tempDir.absoluteFilePath("kscreengenie_kipi.png");
    return QList<QUrl>({ QUrl::fromLocalFile(tempFile) });
}
