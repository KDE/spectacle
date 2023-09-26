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
#include <purpose_version.h>
#include <Purpose/Menu>
#endif

class ExportMenu : public SpectacleMenu
{
    Q_OBJECT

public:
    static ExportMenu *instance();

public Q_SLOTS:
    void openPrintDialog();

Q_SIGNALS:
    void imageShared(int error, const QString &message);

private:
    explicit ExportMenu(QWidget *parent = nullptr);

    Q_SLOT void onImageChanged();
    Q_SLOT void openScreenshotsFolder();

    void getKServiceItems();

#ifdef PURPOSE_FOUND
    void loadPurposeMenu();
    void loadPurposeItems();

    bool mUpdatedImageAvailable;
    std::unique_ptr<Purpose::Menu> mPurposeMenu;
#endif
    friend class ExportMenuSingleton;
};

#endif // EXPORTMENU_H
