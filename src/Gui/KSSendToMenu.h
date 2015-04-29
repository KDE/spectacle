/*
 *  Copyright (C) 2015 Boudhayan Gupta <me@BaloneyGeek.com>
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

#ifndef KSSENDTOMENU_H
#define KSSENDTOMENU_H

#include <QObject>
#include <QWidget>
#include <QString>
#include <QAction>
#include <QList>
#include <QMenu>
#include <QPair>
#include <QVariant>
#include <QDebug>

#include <KLocalizedString>
#include <KService>
#include <KMimeTypeTrader>

#include "Config.h"

#ifdef KIPI_FOUND
#include <KIPI/Interface>
#include <KIPI/PluginLoader>
#include <KIPI/Plugin>

#include "KipiInterface/KSGKipiInterface.h"
#endif

class KSSendToMenu : public QObject
{
    Q_OBJECT

    public:

    explicit KSSendToMenu(QObject *parent = 0);
    ~KSSendToMenu();

    QMenu *menu();

    signals:

    void sendToServiceRequest(KService::Ptr servicePointer);
    void sendToClipboardRequest();
    void sendToOpenWithRequest();

    public slots:

    void populateMenu();

    private slots:

    void handleSendToKService();

    private:

    void populateHardcodedSendToActions();
    void populateKServiceSendToActions();
#ifdef KIPI_FOUND
    void populateKipiSendToActions();

    KSGKipiInterface *mKipiInterface;
    QWidget           mDummyWidget;
#endif
    QMenu            *mMenu;
};

Q_DECLARE_METATYPE(KService::Ptr)

#endif // KSSENDTOMENU_H
