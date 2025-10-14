/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef EXPORTMENU_H
#define EXPORTMENU_H

#include "SpectacleMenu.h"

#include <QMenu>
#include <QQmlEngine>

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
    QML_ELEMENT
    QML_SINGLETON

public:
    static ExportMenu *instance();

    static ExportMenu *create(QQmlEngine *engine, QJSEngine *)
    {
        auto inst = instance();
        Q_ASSERT(inst);
        Q_ASSERT(inst->thread() == engine->thread());
        QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
        return inst;
    }

public Q_SLOTS:
    void openPrintDialog();

Q_SIGNALS:
    void imageShared(int error, const QString &message);

private:
    explicit ExportMenu(QWidget *parent = nullptr);

    Q_SLOT void onImageChanged();
    Q_SLOT void openScreenshotsFolder();
    Q_SLOT void buildOcrLanguageSubmenu();
    Q_SLOT void triggerExtraction(const QString &languageCode);

    void getKServiceItems();
    void createOcrLanguageSubmenu();

#ifdef PURPOSE_FOUND
    void loadPurposeMenu();
    void loadPurposeItems();

    bool mUpdatedImageAvailable;
    std::unique_ptr<Purpose::Menu> mPurposeMenu;
#endif
    QMenu *m_ocrLanguageMenu = nullptr;
    friend class ExportMenuSingleton;
};

#endif // EXPORTMENU_H
