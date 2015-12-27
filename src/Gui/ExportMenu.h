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

#ifndef EXPORTMENU_H
#define EXPORTMENU_H

#include <QMenu>

#include "Config.h"
#include "ExportManager.h"

#ifdef KIPI_FOUND
#include "KipiInterface/KSGKipiInterface.h"
#endif

#ifdef PURPOSE_FOUND
#include <Purpose/AlternativesModel>
#include <PurposeWidgets/Menu>
#endif

class ExportMenu : public QMenu
{
    Q_OBJECT

    public:

    explicit ExportMenu(QWidget *parent = 0);
    void imageUpdated(const QString &dataUri);

    private slots:

    void populateMenu();

    signals:

    void imageShared(bool error, const QString &message);

    private:

    void getKServiceItems();

#ifdef KIPI_FOUND
    void getKipiItems();
    void loadKipiItems();

    bool mKipiMenuLoaded;
    QMenu *mKipiMenu;
    KSGKipiInterface *mKipiInterface;
    QWidget mDummyWidget;
#endif

#ifdef PURPOSE_FOUND
    void loadPurposeMenu();

    Purpose::Menu *mPurposeMenu;
#endif

    ExportManager *mExportManager;
};

#endif // EXPORTMENU_H
