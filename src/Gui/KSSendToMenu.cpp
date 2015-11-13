/*
 *  Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
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

#include "KSSendToMenu.h"

KSSendToMenu::KSSendToMenu(QObject *parent) :
    QObject(parent),
    mMenu(new QMenu)
{}

KSSendToMenu::~KSSendToMenu()
{}

void KSSendToMenu::populateMenu()
{
    populateKServiceSendToActions();
    mMenu->addSeparator();

    QAction *sendToAction = new QAction(this);
    sendToAction->setText(i18n("Other Application"));
    sendToAction->setIcon(QIcon::fromTheme(QStringLiteral("application-x-executable")));
    sendToAction->setShortcuts(KStandardShortcut::open());

    connect(sendToAction, &QAction::triggered, this, &KSSendToMenu::sendToOpenWithRequest);
    mMenu->addAction(sendToAction);
}

// return menu

QMenu *KSSendToMenu::menu()
{
    return mMenu;
}

// send-to handlers

void KSSendToMenu::handleSendToKService()
{
    QAction *action = qobject_cast<QAction *>(QObject::sender());
    if (!(action)) {
        qWarning() << "Internal qobject_cast error. This is a bug.";
        return;
    }

    auto data = action->data().value<KService::Ptr>();
    emit sendToServiceRequest(data);
}

// populators

void KSSendToMenu::populateKServiceSendToActions()
{
    const KService::List services = KMimeTypeTrader::self()->query(QStringLiteral("image/png"));

    for (auto service: services) {
        QString name = service->name().replace('&', QLatin1String("&&"));

        QAction *action = new QAction(QIcon::fromTheme(service->icon()), name, nullptr);
        action->setData(QVariant::fromValue(service));
        connect(action, &QAction::triggered, this, &KSSendToMenu::handleSendToKService);

        mMenu->addAction(action);
    }
}
