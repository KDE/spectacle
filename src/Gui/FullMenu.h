/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "OptionsMenu.h"
#include <QQmlEngine>

/**
 * A menu that allows choosing capture modes and related options.
 */
class FullMenu : public OptionsMenu
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    static FullMenu *instance();

    static FullMenu *create(QQmlEngine *engine, QJSEngine *)
    {
        auto inst = instance();
        Q_ASSERT(inst);
        Q_ASSERT(inst->thread() == engine->thread());
        QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
        return inst;
    }

private:
    explicit FullMenu(QWidget *parent = nullptr);

    friend class FullMenuSingleton;
};
