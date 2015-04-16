/*
 *  Copyright (C) 2015 Boudhayan Gupta <me@BaloneyGeek.com>
 *
 *  Contains code from ksnapshotsendtoactions.cpp, part of KSnapshot.
 *  Copyright notices reproduced below:
 *
 *  Copyright (C) 2014 Gregor Mi <codestruct@posteo.org>
 *  moved from ksnapshot.h, see authors there
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

#include "SendToActionsPopulator.h"

SendToActionsPopulator::SendToActionsPopulator(QObject *parent) : QObject(parent)
{

}

SendToActionsPopulator::~SendToActionsPopulator()
{

}

void SendToActionsPopulator::process()
{
    sendHardcodedSendToActions();
    emit haveSeperator();
    sendKServiceSendToActions();
#ifdef HAVE_KIPI
    emit haveSeperator();
    sendKipiSendToActions();
#endif
    emit allDone();
}

void SendToActionsPopulator::sendHardcodedSendToActions()
{
    const QVariant data_clip = QVariant::fromValue(ActionData(HardcodedAction, "clipboard"));
    const QVariant data_app = QVariant::fromValue(ActionData(HardcodedAction, "application"));

    emit haveAction(QIcon::fromTheme("edit-copy"), i18n("Copy To Clipboard"), data_clip);
    emit haveAction(QIcon(), i18n("Other Application"), data_app);
}

void SendToActionsPopulator::sendKServiceSendToActions()
{
    const KService::List services = KMimeTypeTrader::self()->query("image/png");

    for (auto service: services) {
        QString name = service->name().replace('&', "&&");
        const QVariant data = QVariant::fromValue(ActionData(KServiceAction, service->menuId()));

        emit haveAction(QIcon::fromTheme(service->icon()), name, data);
    }
}

#ifdef HAVE_KIPI
void SendToActionsPopulator::sendKipiSendToActions()
{
    ;;
}
#endif
