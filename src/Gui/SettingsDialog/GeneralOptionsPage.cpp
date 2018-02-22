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

#include <QGroupBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>

#include <KLocalizedString>

#include "SpectacleConfig.h"

GeneralOptionsPage::GeneralOptionsPage(QWidget *parent) :
    SettingsPage(parent)
{
    // preamble and stuff

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // copy save path to clipboard

    mCopyPathToClipboard = new QCheckBox(i18n("Copy save location to the clipboard"), this);
    connect(mCopyPathToClipboard, &QCheckBox::toggled, this, &GeneralOptionsPage::markDirty);
    mainLayout->addWidget(mCopyPathToClipboard, 1);

    // Rectangular Region settings

    QGroupBox *rrGroup = new QGroupBox(i18n("Rectangular Region"));
    QVBoxLayout *rrLayout = new QVBoxLayout;
    rrGroup->setLayout(rrLayout);

    // use light background

    mUseLightBackground = new QCheckBox(i18n("Use light background"), this);
    connect(mUseLightBackground, &QCheckBox::toggled, this, &GeneralOptionsPage::markDirty);
    mainLayout->addWidget(mUseLightBackground, 1);

    // remember Rectangular Region box

    mRememberRect = new QCheckBox(i18n("Remember selected area"), this);
    connect(mRememberRect, &QCheckBox::toggled, this, &GeneralOptionsPage::markDirty);

    QVBoxLayout *rrCLayout = new QVBoxLayout;
//     rrCLayout->setContentsMargins(15, 10, 0, 10);
    rrCLayout->addWidget(mUseLightBackground);
    rrCLayout->addWidget(mRememberRect);
    rrLayout->addLayout(rrCLayout);
    mainLayout->addWidget(rrGroup, 1);

    // read in the data

    resetChanges();

    // finish up with the main layout

    mainLayout->addStretch(4);
    setLayout(mainLayout);
}

void GeneralOptionsPage::markDirty(bool checked)
{
    Q_UNUSED(checked);
    mChangesMade = true;
}

void GeneralOptionsPage::saveChanges()
{
    SpectacleConfig *cfgManager = SpectacleConfig::instance();

    cfgManager->setUseLightRegionMaskColour(mUseLightBackground->checkState() == Qt::Checked);
    cfgManager->setRememberLastRectangularRegion(mRememberRect->checkState() == Qt::Checked);
    cfgManager->setCopySaveLocationToClipboard(mCopyPathToClipboard->checkState() == Qt::Checked);

    mChangesMade = false;
}

void GeneralOptionsPage::resetChanges()
{
    SpectacleConfig *cfgManager = SpectacleConfig::instance();

    mUseLightBackground->setChecked(cfgManager->useLightRegionMaskColour());
    mRememberRect->setChecked(cfgManager->rememberLastRectangularRegion());
    mCopyPathToClipboard->setChecked(cfgManager->copySaveLocationToClipboard());

    mChangesMade = false;
}
