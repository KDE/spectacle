/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "SpectacleMenu.h"

class TextContextMenu : public SpectacleMenu
{
    Q_OBJECT

public:
    explicit TextContextMenu(QWidget *parent = nullptr);

    void popup(QQuickItem *editor) override;

private:
    QAction *undoAction = nullptr;
    QAction *redoAction = nullptr;
    QAction *cutAction = nullptr;
    QAction *copyAction = nullptr;
    QAction *pasteAction = nullptr;
    QAction *deleteAction = nullptr;
    QAction *selectAllAction = nullptr;
};
