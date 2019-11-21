/*
 * Copyright (C) 2019  David Redondo <kde@david-redondo.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef SHORTCUT_ACTIONS_H
#define SHORTCUT_ACTIONS_H

#include <KActionCollection>

class ShortcutActions {
public:
    static ShortcutActions* self();

    KActionCollection* shortcutActions();

    QString componentName() const;

    QAction* openAction() const;
    QAction* fullScreenAction() const;
    QAction* currentScreenAction() const;
    QAction* activeWindowAction() const;
    QAction* regionAction() const;

private:
    ShortcutActions();
    KActionCollection mActions;
};

#endif
