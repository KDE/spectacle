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

#include "GeneralOptionsPage.h"

#include "SpectacleConfig.h"

#include <KLocalizedString>

#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>

GeneralOptionsPage::GeneralOptionsPage(QWidget *parent) :
    SettingsPage(parent)
{
    QFormLayout *mainLayout = new QFormLayout(this);
    setLayout(mainLayout);

    // Rectangular Region settings

    // use light background
    mUseLightBackground = new QCheckBox(i18n("Use light background"), this);
    connect(mUseLightBackground, &QCheckBox::toggled, this, &GeneralOptionsPage::markDirty);
    mainLayout->addRow(i18n("Rectangular Region:"), mUseLightBackground);

    // remember Rectangular Region box
    mRememberRect = new QCheckBox(i18n("Remember selected area"), this);
    connect(mRememberRect, &QCheckBox::toggled, this, &GeneralOptionsPage::markDirty);
    mainLayout->addRow(QString(), mRememberRect);

    // show magnifier
    mShowMagnifier = new QCheckBox(i18n("Show magnifier"), this);
    connect(mShowMagnifier, &QCheckBox::toggled, this, &GeneralOptionsPage::markDirty);
    mainLayout->addRow(QString(), mShowMagnifier);

    // read in the data
    resetChanges();
}

void GeneralOptionsPage::markDirty()
{
    mChangesMade = true;
}

void GeneralOptionsPage::saveChanges()
{
    SpectacleConfig *cfgManager = SpectacleConfig::instance();

    cfgManager->setUseLightRegionMaskColour(mUseLightBackground->checkState() == Qt::Checked);
    cfgManager->setRememberLastRectangularRegion(mRememberRect->checkState() == Qt::Checked);
    cfgManager->setShowMagnifierChecked(mShowMagnifier->checkState() == Qt::Checked);

    mChangesMade = false;
}

void GeneralOptionsPage::resetChanges()
{
    SpectacleConfig *cfgManager = SpectacleConfig::instance();

    mUseLightBackground->setChecked(cfgManager->useLightRegionMaskColour());
    mRememberRect->setChecked(cfgManager->rememberLastRectangularRegion());
    mShowMagnifier->setChecked(cfgManager->showMagnifierChecked());

    mChangesMade = false;
}
