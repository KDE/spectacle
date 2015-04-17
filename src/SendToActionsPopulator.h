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

#ifndef SENDTOACTIONSPOPULATOR_H
#define SENDTOACTIONSPOPULATOR_H

#include <QObject>
#include <QString>
#include <QAction>
#include <QList>
#include <QPair>
#include <QVariant>
#include <QMenu>

#include "Config.h"

#include <KLocalizedString>
#include <KService>
#include <KMimeTypeTrader>

class SendToActionsPopulator : public QObject
{
    Q_OBJECT
    Q_ENUMS(SendToActionType)

    public:

    enum SendToActionType {
        HardcodedAction,
        KServiceAction,
        KipiAction
    };

    explicit SendToActionsPopulator(QObject *parent = 0);
    ~SendToActionsPopulator();

    signals:

    void haveAction(const QIcon, const QString, const QVariant);
    void haveSeperator();
    void allDone();

    public slots:

    void process();

    private:

    void sendHardcodedSendToActions();
    void sendKServiceSendToActions();
#ifdef HAVE_KIPI
    void sendKipiSendToActions();
#endif
};

typedef QPair<SendToActionsPopulator::SendToActionType, QString> ActionData;
Q_DECLARE_METATYPE(ActionData)

#endif // SENDTOACTIONSPOPULATOR_H
