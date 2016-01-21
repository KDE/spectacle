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

#include <QMetaObject>
#include <QIcon>

#include <KLocalizedString>
#include <KPageWidgetModel>

#include "SaveOptionsPage.h"
#include "GeneralOptionsPage.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    KPageDialog(parent)
{
    // set up window options and geometry
    setWindowTitle(i18n("Preferences"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(500, 700);

    // init all pages
    QMetaObject::invokeMethod(this, "initPages", Qt::QueuedConnection);
}

SettingsDialog::~SettingsDialog()
{}

void SettingsDialog::initPages()
{
    KPageWidgetItem *generalOptions = new KPageWidgetItem(new GeneralOptionsPage(this), i18n("General"));
    generalOptions->setHeader(i18n("General application preferences"));
    generalOptions->setIcon(QIcon::fromTheme(QStringLiteral("applications-system")));
    addPage(generalOptions);
    mPages.insert(generalOptions);

    KPageWidgetItem *saveOptions = new KPageWidgetItem(new SaveOptionsPage(this), i18n("Save Options"));
    saveOptions->setHeader(i18n("Default save settings"));
    saveOptions->setIcon(QIcon::fromTheme(QStringLiteral("document-save")));
    addPage(saveOptions);
    mPages.insert(saveOptions);
}

void SettingsDialog::accept()
{
    Q_FOREACH(auto page, mPages) {
        SettingsPage *pageWidget = dynamic_cast<SettingsPage *>(page->widget());
        if (pageWidget) {
            pageWidget->saveChanges();
        }
    }

    emit done(QDialog::Accepted);
}
