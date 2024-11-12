/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "SpectacleMenu.h"

#include <KHelpMenu>

#include <QQmlEngine>

#include <memory>

class HelpMenu : public SpectacleMenu
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    static HelpMenu *instance();

    Q_SLOT void showAppHelp();

    static HelpMenu *create(QQmlEngine *engine, QJSEngine *)
    {
        auto inst = instance();
        Q_ASSERT(inst);
        Q_ASSERT(inst->thread() == engine->thread());
        QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
        return inst;
    }

private:
    explicit HelpMenu(QWidget *parent = nullptr);
    Q_SLOT void onTriggered(QAction *action);
    const std::unique_ptr<KHelpMenu> kHelpMenu;
    friend class HelpMenuSingleton;
};
