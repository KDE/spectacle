/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <me@BaloneyGeek.com>
 *  SPDX-FileCopyrightText: 2010 Pau Garcia i Quiles <pgquiles@elpauer.org>
 *  Essentially a rip-off of code for Kamoso by:
 *  SPDX-FileCopyrightText: 2008-2009 Aleix Pol <aleixpol@kde.org>
 *  SPDX-FileCopyrightText: 2008-2009 Alex Fiestas <alex@eyeos.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "KSGKipiImageCollectionShared.h"
#include "ExportManager.h"

KSGKipiImageCollectionShared::KSGKipiImageCollectionShared()  {}
KSGKipiImageCollectionShared::~KSGKipiImageCollectionShared() {}

QString KSGKipiImageCollectionShared::name()           { return QStringLiteral("Spectacle"); }
QString KSGKipiImageCollectionShared::comment()        { return QString(); }
QString KSGKipiImageCollectionShared::uploadRootName() { return QStringLiteral("/"); }
QUrl    KSGKipiImageCollectionShared::uploadRoot()     { return QUrl(uploadRootName()); }
bool    KSGKipiImageCollectionShared::isDirectory()    { return false; }
QList<QUrl> KSGKipiImageCollectionShared::images()     { return QList<QUrl>({ ExportManager::instance()->tempSave() }); }

