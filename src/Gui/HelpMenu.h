/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "SpectacleMenu.h"

#include <KHelpMenu>

#include <memory>

class HelpMenu : public SpectacleMenu
{
    Q_OBJECT

public:
    static HelpMenu *instance();

    Q_SLOT void showAppHelp();

private:
    explicit HelpMenu(QWidget *parent = nullptr);
    Q_SLOT void onTriggered(QAction *action);
    const std::unique_ptr<KHelpMenu> kHelpMenu;
    friend class HelpMenuSingleton;
};
