/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef OPTIONSMENU_H
#define OPTIONSMENU_H

#include "SpectacleMenu.h"
#include "Gui/SmartSpinBox.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QQmlEngine>
#include <QWidgetAction>

/**
 * A menu that allows choosing capture modes and related options.
 */
class OptionsMenu : public SpectacleMenu
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    static OptionsMenu *instance();

    Q_SLOT void showPreferencesDialog();

    static OptionsMenu *create(QQmlEngine *engine, QJSEngine *)
    {
        auto inst = instance();
        Q_ASSERT(inst);
        Q_ASSERT(inst->thread() == engine->thread());
        QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
        return inst;
    }

protected:
    void changeEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    explicit OptionsMenu(QWidget *parent = nullptr);
    ~OptionsMenu();

    void delayActionLayoutUpdate();
    QWidgetAction *const m_delayAction;
    QWidget *const m_delayWidget;
    QHBoxLayout *const m_delayLayout;
    QLabel *const m_delayLabel;
    SmartSpinBox *const m_delaySpinBox;
    bool m_updatingDelayActionLayout = false;
};

#endif // OPTIONSMENU_H
