/*
 *  SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "SettingsDialog.h"

#include "GeneralOptionsPage.h"
#include "SaveOptionsPage.h"
#include "settings.h"
#include "ShortcutsOptionsPage.h"

#include <KLocalizedString>
#include <KShortcutWidget>


SettingsDialog::SettingsDialog(QWidget *parent) :
    KConfigDialog(parent, QStringLiteral("settings"), Settings::self())
{
    addPage(new GeneralOptionsPage(this), Settings::self(),
        i18n("General"),  QStringLiteral("spectacle"));
    addPage(new SaveOptionsPage(this), Settings::self(),
        i18n("Save"), QStringLiteral("document-save"));
    mShortcutsPage = new ShortcutsOptionsPage(this);
    addPage(mShortcutsPage, i18n("Shortcuts"), QStringLiteral("preferences-desktop-keyboard"));
    connect(mShortcutsPage, &ShortcutsOptionsPage::shortCutsChanged, this, [this] {
        updateButtons();
    });
    resize(600, 590);
    connect(this, &KConfigDialog::currentPageChanged, this, &SettingsDialog::updateButtons);
}

bool SettingsDialog::hasChanged()
{
    return mShortcutsPage->isModified() || KConfigDialog::hasChanged();
}

bool SettingsDialog::isDefault()
{
    return currentPage()->name() !=  i18n("Shortcuts") && KConfigDialog::isDefault();
}

void SettingsDialog::updateSettings()
{
    KConfigDialog::updateSettings();
    mShortcutsPage->saveChanges();
}

void SettingsDialog::updateWidgets()
{
    KConfigDialog::updateWidgets();
    mShortcutsPage->resetChanges();
}

void SettingsDialog::updateWidgetsDefault()
{
    KConfigDialog::updateWidgetsDefault();
    mShortcutsPage->defaults();
}





