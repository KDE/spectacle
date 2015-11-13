/*
 *  Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
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

#include "ExportMenu.h"

#include <QTimer>
#include <QList>
#include <QUrl>

#include <KLocalizedString>
#include <KStandardShortcut>
#include <KService>
#include <KMimeTypeTrader>
#include <KRun>

ExportMenu::ExportMenu(QWidget *parent) :
    QMenu(parent),
    mExportManager(ExportManager::instance())
{
    QTimer::singleShot(300, this, &ExportMenu::populateMenu);
}

void ExportMenu::populateMenu()
{
    return getKServiceItems();
}

void ExportMenu::getKServiceItems()
{
    //addSection(i18n("Local Applications"));

    // populate all locally installed applications and services
    // which can handle images first

    const KService::List services = KMimeTypeTrader::self()->query(QStringLiteral("image/png"));

    Q_FOREACH (auto service, services) {
        QString name = service->name().replace('&', QLatin1String("&&"));
        QAction *action = new QAction(QIcon::fromTheme(service->icon()), name, nullptr);

        connect(action, &QAction::triggered, [=]() {
            QList<QUrl> whereIs({ mExportManager->tempSave() });
            KRun::runService(*service, whereIs, parentWidget(), true);
        });
        addAction(action);
    }

    // now let the user manually chose an application to open the
    // image with

    QAction *openWith = new QAction(this);
    openWith->setText(i18n("Other Application"));
    openWith->setIcon(QIcon::fromTheme(QStringLiteral("application-x-executable")));
    openWith->setShortcuts(KStandardShortcut::open());

    connect(openWith, &QAction::triggered, [=]() {
        QList<QUrl> whereIs({ mExportManager->tempSave() });
        KRun::displayOpenWithDialog(whereIs, parentWidget(), true);
    });
    addAction(openWith);
}
