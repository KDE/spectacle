/*
 * SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SHORTCUT_ACTIONS_H
#define SHORTCUT_ACTIONS_H

#include <KActionCollection>

class ShortcutActions
{
public:
    static ShortcutActions *self();

    KActionCollection *shortcutActions();

    QString componentName() const;

    QAction *openAction() const;
    QAction *fullScreenAction() const;
    QAction *currentScreenAction() const;
    QAction *activeWindowAction() const;
    QAction *regionAction() const;
    QAction *windowUnderCursorAction() const;

private:
    ShortcutActions();
    KActionCollection mActions;
};

#endif
