/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
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

    explicit ExportMenu(QWidget *parent = nullptr);
    void imageUpdated();

    private Q_SLOTS:

    void populateMenu();

    Q_SIGNALS:

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
    void loadPurposeItems();

    bool mUpdatedImageAvailable;
    Purpose::Menu *mPurposeMenu;
#endif

    ExportManager *mExportManager;
};

#endif // EXPORTMENU_H
