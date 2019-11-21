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

#include "GeneralOptionsPage.h"

#include <KLocalizedString>
#include <KTitleWidget>
#include <KWindowSystem>

#include <QButtonGroup>
#include <QCheckBox>
#include <QLineEdit>
#include <QFormLayout>
#include <QRadioButton>
#include <QSpacerItem>
#include <QTextEdit>

GeneralOptionsPage::GeneralOptionsPage(QWidget *parent) :
    QWidget{parent}
{
    QFormLayout *mainLayout = new QFormLayout(this);
    setLayout(mainLayout);

    // When spectacle is running settings
    KTitleWidget* runningTitle = new KTitleWidget(this);
    runningTitle->setText(i18n("When Spectacle is Running"));
    runningTitle->setLevel(2);
    mainLayout->addRow(runningTitle);
    QRadioButton* takeNew = new QRadioButton(i18n("Take a new screenshot"), this);
    QRadioButton* startNewInstance = new QRadioButton(i18n("Open a new Spectacle window"), this);
    QButtonGroup* printKeyActionGroup = new QButtonGroup(this);
    printKeyActionGroup->setExclusive(true);
    printKeyActionGroup->addButton(takeNew,0);// SpectacleConfig::PrintKeyActionRunning::TakeNewScreenshot);
    printKeyActionGroup->addButton(startNewInstance,1);// SpectacleConfig::PrintKeyActionRunning::StartNewInstance);
    mainLayout->addRow(i18n("Press screenshot key to:"), takeNew);
    mainLayout->addRow(QString(), startNewInstance);
    //On Wayland  we can't programmatically raise and focus the window so we have to hide the option
    if (!(KWindowSystem::isPlatformWayland() || qstrcmp(qgetenv("XDG_SESSION_TYPE"), "wayland") == 0)) {
        QRadioButton* focusWindow = new QRadioButton(i18n("Return focus to Spectacle"), this);
        printKeyActionGroup->addButton( focusWindow,2);// SpectacleConfig::PrintKeyActionRunning::FocusWindow);
        mainLayout->addRow(QString(), focusWindow);
    }

    //Workaround because KConfigWidgets doesn't support QButtonGroup (Bug 409037)
    auto workaroundLabel = new QLineEdit(this);
    workaroundLabel->setHidden(true);
    workaroundLabel->setObjectName(QStringLiteral("kcfg_printKeyActionRunning"));
    // Need to check default Button because we get no change event for that
    takeNew->setChecked(true);
    connect(workaroundLabel, &QLineEdit::textChanged,
            printKeyActionGroup, [printKeyActionGroup, takeNew](const QString& text){
                auto button = printKeyActionGroup->button(text.toInt());
                // We are missing a button on Wayland
                button ? button->setChecked(true) : takeNew->setChecked(true);
    });
    connect(printKeyActionGroup, qOverload<int, bool>(&QButtonGroup::buttonToggled),
            workaroundLabel, [workaroundLabel] (int value, bool checked) {
                if (checked) {
                    workaroundLabel->setText(QString::number(value));
                }
    });
    // /Workaround

    mainLayout->addItem(new QSpacerItem(0, 18, QSizePolicy::Fixed, QSizePolicy::Fixed));

    // actions to take after taking a screenshot
    auto copyImageToClipboard = new QCheckBox(i18n("Copy image to clipboard"), this);
    copyImageToClipboard->setObjectName(QStringLiteral("kcfg_copyImageToClipboard"));
    mainLayout->addRow(i18n("After taking a screenshot:"), copyImageToClipboard);

    auto autoSaveImage = new QCheckBox(i18n("Autosave the image to the default location"), this);
    autoSaveImage->setObjectName(QStringLiteral("kcfg_autoSaveImage"));
    mainLayout->addRow(QString(), autoSaveImage);

    mainLayout->addItem(new QSpacerItem(0, 18, QSizePolicy::Fixed, QSizePolicy::Fixed));

    // Rectangular Region settings
    KTitleWidget *titleWidget = new KTitleWidget(this);
    titleWidget->setText(i18n("Rectangular Region"));
    titleWidget->setLevel(2);
    mainLayout->addRow(titleWidget);

    // use light background
    QCheckBox* kcfg_useLightMaskColour = new QCheckBox(i18n("Use light background"), this);
    kcfg_useLightMaskColour->setObjectName(QStringLiteral("kcfg_useLightMaskColour"));
    mainLayout->addRow(i18n("General:"), kcfg_useLightMaskColour);

    // show magnifier
    auto showMagnifier = new QCheckBox(i18n("Show magnifier"), this);
    showMagnifier->setObjectName(QStringLiteral("kcfg_showMagnifier"));
    mainLayout->addRow(QString(), showMagnifier);

    // release mouse-button to capture
    auto releaseToCapture = new QCheckBox(i18n("Accept on click-and-release"), this);
    releaseToCapture->setObjectName(QStringLiteral("kcfg_useReleaseToCapture"));
    mainLayout->addRow(QString(), releaseToCapture);

    mainLayout->addItem(new QSpacerItem(0, 18, QSizePolicy::Fixed, QSizePolicy::Fixed));

    // remember Rectangular Region box
    QButtonGroup* rememberGroup = new QButtonGroup(this);
    rememberGroup->setExclusive(true);
    QRadioButton* neverButton = new QRadioButton(i18n("Never"), this);
    auto rememberAlways = new QRadioButton(i18n("Always"), this);
    rememberAlways->setObjectName(QStringLiteral("kcfg_alwaysRememberRegion"));
    auto rememberUntilClosed = new QRadioButton(i18n("Until Spectacle is closed"), this);
    rememberUntilClosed->setObjectName(QStringLiteral("kcfg_rememberLastRectangularRegion"));
    rememberGroup->addButton(neverButton);
    rememberGroup->addButton(rememberAlways);
    rememberGroup->addButton(rememberUntilClosed);
    mainLayout->addRow(i18n("Remember selected area:"), neverButton);

    mainLayout->addRow(QString(), rememberAlways);
    mainLayout->addRow(QString(), rememberUntilClosed);
}
