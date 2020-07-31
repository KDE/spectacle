/*
 *  Copyright 2019 David Redondo <kde@david-redondo.de>
 *  Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
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





