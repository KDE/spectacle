/* SPDX-FileCopyrightText: 2026 tomek7667 <git@cyber-man.pl>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "SpectacleMenu.h"

#include <QQmlEngine>

class AudioDeviceModel;

/**
 * A menu of checkable audio devices to pick the ones that get recorded.
 */
class AudioDeviceMenu : public SpectacleMenu
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit AudioDeviceMenu(AudioDeviceModel *model, QWidget *parent = nullptr);

    /// The menu for the shared model that is bound to the settings
    static AudioDeviceMenu *instance();

    static AudioDeviceMenu *create(QQmlEngine *engine, QJSEngine *)
    {
        auto inst = instance();
        Q_ASSERT(inst);
        Q_ASSERT(inst->thread() == engine->thread());
        QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
        return inst;
    }

private:
    void rebuild();
    void updateCheckedStates();

    AudioDeviceModel *const m_model;
    QList<QAction *> m_deviceActions;
};
