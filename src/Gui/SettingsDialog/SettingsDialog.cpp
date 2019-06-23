/*
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
#include "ShortcutsOptionsPage.h"

#include <KLocalizedString>
#include <KShortcutWidget>

#include <QIcon>
#include <QMessageBox>

SettingsDialog::SettingsDialog(QWidget *parent) :
    KPageDialog(parent)
{
    // set up window options and geometry
    setWindowTitle(i18nc("@title:window", "Configure"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(600, 550);

    // init all pages
    QMetaObject::invokeMethod(this, "initPages", Qt::QueuedConnection);
}

void SettingsDialog::initPages()
{
    KPageWidgetItem *generalOptions = new KPageWidgetItem(new GeneralOptionsPage(this), i18n("General"));
    generalOptions->setHeader(i18n("General"));
    generalOptions->setIcon(QIcon::fromTheme(QStringLiteral("view-preview"))); // This is what Dolphin uses for the icon on its General page...
    addPage(generalOptions);
    mPages.insert(generalOptions);

    KPageWidgetItem *saveOptions = new KPageWidgetItem(new SaveOptionsPage(this), i18n("Save"));
    saveOptions->setHeader(i18n("Save"));
    saveOptions->setIcon(QIcon::fromTheme(QStringLiteral("document-save")));
    addPage(saveOptions);
    mPages.insert(saveOptions);

    KPageWidgetItem *shortcutOptions = new KPageWidgetItem(new ShortcutsOptionsPage(this), i18n("Shortcuts"));
    shortcutOptions->setHeader(i18n("Shortcuts"));
    shortcutOptions->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-keyboard")));
    addPage(shortcutOptions);
    mPages.insert(shortcutOptions);

    connect(this, &SettingsDialog::currentPageChanged, this, &SettingsDialog::onPageChanged);
}

void SettingsDialog::accept()
{
    for (auto page : qAsConst(mPages)) {
        SettingsPage *pageWidget = dynamic_cast<SettingsPage *>(page->widget());
        if (pageWidget) {
            pageWidget->saveChanges();
        }
    }

    done(QDialog::Accepted);
}

void SettingsDialog::onPageChanged(KPageWidgetItem *current, KPageWidgetItem *before)
{
    Q_UNUSED(current);

    SettingsPage *pageWidget = dynamic_cast<SettingsPage *>(before->widget());
    if (pageWidget && (pageWidget->changesMade())) {
        QMessageBox::StandardButton response = QMessageBox::question(this, i18n("Apply Unsaved Changes"),
                     i18n("You have made changes to the settings in this tab. Do you want to apply those changes?"));

        if (response == QMessageBox::Yes) {
            pageWidget->saveChanges();
        } else {
            pageWidget->resetChanges();
        }
    }
}
