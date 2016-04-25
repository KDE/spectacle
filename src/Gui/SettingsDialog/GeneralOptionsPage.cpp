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

    // dynamic save button

    QGroupBox *dsGroup = new QGroupBox(i18n("Dynamic Save Button"));
    QVBoxLayout *dsLayout = new QVBoxLayout;
    dsGroup->setLayout(dsLayout);
    dsGroup->setStyleSheet(QStringLiteral("QGroupBox { font-weight: bold; }"));

    QLabel *dsHelpText = new QLabel;
    dsHelpText->setWordWrap(true);
    dsHelpText->setText(i18n("The default behavior of the save button is to <b>Save & Exit</b>. Enable this "
                             "option to change the save button to either <b>Save</b>, <b>Save As</b> or "
                             "<b>Save & Exit</b>, according to the save action you perform, every time you "
                             "save an image."));
    dsLayout->addWidget(dsHelpText);

    mUseLastSaveAction = new QCheckBox;
    mUseLastSaveAction->setText(i18n("Set save button action to last used save method"));
    connect(mUseLastSaveAction, &QCheckBox::toggled, this, &GeneralOptionsPage::markDirty);

    QVBoxLayout *dsCLayout = new QVBoxLayout;
    dsCLayout->setContentsMargins(15, 10, 0, 10);
    dsCLayout->addWidget(mUseLastSaveAction);
    dsLayout->addLayout(dsCLayout);
    mainLayout->addWidget(dsGroup, 1);

    // copy save path to clipboard

    QGroupBox *cpGroup = new QGroupBox(i18n("Copy Save Location to Clipboard"));
    QVBoxLayout *cpLayout = new QVBoxLayout;
    cpGroup->setLayout(cpLayout);
    cpGroup->setStyleSheet(QStringLiteral("QGroupBox { font-weight: bold; }"));

    QLabel *cpHelpText = new QLabel;
    cpHelpText->setWordWrap(true);
    cpHelpText->setText(i18n("When a screenshot is saved, copy the location at which the file was saved "
                             "to the clipboard. You can then paste it anywhere that accepts text input. "
                             "Note that you must be running a clipboard manager in order to keep the path "
                             "in the clipboard after Spectacle quits."));
    cpLayout->addWidget(cpHelpText);

    mCopyPathToClipboard = new QCheckBox;
    mCopyPathToClipboard->setText(i18n("Copy save location to the clipboard"));
    connect(mCopyPathToClipboard, &QCheckBox::toggled, this, &GeneralOptionsPage::markDirty);

    QVBoxLayout *cpCLayout = new QVBoxLayout;
    cpCLayout->setContentsMargins(15, 10, 0, 10);
    cpCLayout->addWidget(mCopyPathToClipboard);
    cpLayout->addLayout(cpCLayout);
    mainLayout->addWidget(cpGroup, 1);

    // use a light background for the rectangular region grabber

    QGroupBox *lbGroup = new QGroupBox(i18n("Light Background For Rectangular Region"));
    QVBoxLayout *lbLayout = new QVBoxLayout;
    lbGroup->setLayout(lbLayout);
    lbGroup->setStyleSheet(QStringLiteral("QGroupBox { font-weight: bold; }"));

    QLabel *lbHelpText = new QLabel;
    lbHelpText->setWordWrap(true);
    lbHelpText->setText(i18n("Use a light background color to mask the cropped-out area in the rectangular "
                             "region selector. This may make dark cursors easier to see."));
    lbLayout->addWidget(lbHelpText);

    mUseLightBackground = new QCheckBox;
    mUseLightBackground->setText(i18n("Use light background color"));
    connect(mUseLightBackground, &QCheckBox::toggled, this, &GeneralOptionsPage::markDirty);

    QVBoxLayout *lbCLayout = new QVBoxLayout;
    lbCLayout->setContentsMargins(15, 10, 0, 10);
    lbCLayout->addWidget(mUseLightBackground);
    lbLayout->addLayout(lbCLayout);
    mainLayout->addWidget(lbGroup, 1);

    // remember rectangular region

    QGroupBox *rrGroup = new QGroupBox(i18n("Remember Rectangular Region"));
    QVBoxLayout *rrLayout = new QVBoxLayout;
    rrGroup->setLayout(rrLayout);
    rrGroup->setStyleSheet(QStringLiteral("QGroupBox { font-weight: bold; }"));

    QLabel *rrHelpText = new QLabel;
    rrHelpText->setWordWrap(true);
    rrHelpText->setText(i18n("By default, Spectacle does not show an initial selection when you take a "
                             "screenshot of a rectangular region. Enable this option to remember the last "
                             "selected region of the screen, and set it as the initial selection when you "
                             "use the rectangular region selector the next time."));
    rrLayout->addWidget(rrHelpText);

    mRememberRect = new QCheckBox;
    mRememberRect->setText(i18n("Remember rectangular region"));
    connect(mRememberRect, &QCheckBox::toggled, this, &GeneralOptionsPage::markDirty);

    QVBoxLayout *rrCLayout = new QVBoxLayout;
    rrCLayout->setContentsMargins(15, 10, 0, 10);
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

    cfgManager->setUseDynamicSaveButton(mUseLastSaveAction->checkState() == Qt::Checked);
    cfgManager->setUseLightRegionMaskColour(mUseLightBackground->checkState() == Qt::Checked);
    cfgManager->setRememberLastRectangularRegion(mRememberRect->checkState() == Qt::Checked);
    cfgManager->setCopySaveLocationToClipboard(mCopyPathToClipboard->checkState() == Qt::Checked);

    mChangesMade = false;
}

void GeneralOptionsPage::resetChanges()
{
    SpectacleConfig *cfgManager = SpectacleConfig::instance();

    mUseLastSaveAction->setChecked(cfgManager->useDynamicSaveButton());
    mUseLightBackground->setChecked(cfgManager->useLightRegionMaskColour());
    mRememberRect->setChecked(cfgManager->rememberLastRectangularRegion());
    mCopyPathToClipboard->setChecked(cfgManager->copySaveLocationToClipboard());

    mChangesMade = false;
}
