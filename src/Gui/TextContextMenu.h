/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "SpectacleMenu.h"
#include <QQmlEngine>

class TextContextMenu : public SpectacleMenu
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    static TextContextMenu *instance();

    static TextContextMenu *create(QQmlEngine *engine, QJSEngine *)
    {
        auto inst = instance();
        Q_ASSERT(inst);
        Q_ASSERT(inst->thread() == engine->thread());
        QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
        return inst;
    }

    void popup(QQuickItem *editor) override;

private:
    explicit TextContextMenu(QWidget *parent = nullptr);

    QAction *undoAction = nullptr;
    QAction *redoAction = nullptr;
    QAction *cutAction = nullptr;
    QAction *copyAction = nullptr;
    QAction *pasteAction = nullptr;
    QAction *deleteAction = nullptr;
    QAction *selectAllAction = nullptr;

    friend class TextContextMenuSingleton;
};
