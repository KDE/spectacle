/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef OPTIONSMENU_H
#define OPTIONSMENU_H

#include "SpectacleMenu.h"

#include "Gui/SmartSpinBox.h"

#include <QActionGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QWidgetAction>

#include <memory>

/**
 * A menu that allows choosing capture modes and related options.
 */
class OptionsMenu : public SpectacleMenu
{
    Q_OBJECT

public:
    static OptionsMenu *instance();

    Q_SLOT void showPreferencesDialog();

    void setCaptureModeOptionsEnabled(bool enabled);

protected:
    void changeEvent(QEvent *event) override;

private:
    explicit OptionsMenu(QWidget *parent = nullptr);

    void delayActionLayoutUpdate();
    Q_SLOT void updateCaptureModes();

    QList<QAction *> captureModeActions;
    const std::unique_ptr<QAction> captureModeSection;
    const std::unique_ptr<QActionGroup> captureModeGroup;
    const std::unique_ptr<QAction> captureSettingsSection;
    const std::unique_ptr<QAction> includeMousePointerAction;
    const std::unique_ptr<QAction> includeWindowDecorationsAction;
    const std::unique_ptr<QAction> includeWindowShadowAction;
    const std::unique_ptr<QAction> onlyCapturePopupAction;
    const std::unique_ptr<QAction> quitAfterSaveAction;
    const std::unique_ptr<QAction> captureOnClickAction;
    const std::unique_ptr<QWidgetAction> delayAction;
    const std::unique_ptr<QWidget> delayWidget;
    const std::unique_ptr<QHBoxLayout> delayLayout;
    const std::unique_ptr<QLabel> delayLabel;
    const std::unique_ptr<SmartSpinBox> delaySpinBox;

    bool captureModesInitialized = false;
    bool shouldUpdateCaptureModes = true;
    bool updatingDelayActionLayout = false;
    bool captureModeOptionsEnabled = true;

    friend class OptionsMenuSingleton;
};

#endif // OPTIONSMENU_H
