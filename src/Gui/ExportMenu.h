/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef EXPORTMENU_H
#define EXPORTMENU_H

#include "SpectacleMenu.h"

#include "Config.h"
#include "ExportManager.h"

#ifdef PURPOSE_FOUND
#include <Purpose/AlternativesModel>
#include <PurposeWidgets/Menu>
#endif

class ExportMenu : public SpectacleMenu
{
    Q_OBJECT

public:
    explicit ExportMenu(QWidget *parent = nullptr);

public Q_SLOTS:
    void openPrintDialog();

Q_SIGNALS:
    void imageShared(int error, const QString &message);

private:
    Q_SLOT void onPixmapChanged();
    Q_SLOT void openScreenshotsFolder();

    void getKServiceItems();

#ifdef PURPOSE_FOUND
    void loadPurposeMenu();
    void loadPurposeItems();

    bool mUpdatedImageAvailable;
    Purpose::Menu *mPurposeMenu = nullptr;
#endif
};

#endif // EXPORTMENU_H
