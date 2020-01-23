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

#include "settings.h"
#include "ui_GeneralOptions.h"

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

GeneralOptionsPage::GeneralOptionsPage(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui_GeneralOptions)
{
    m_ui->setupUi(this);

    m_ui->runningTitle->setLevel(2);
    m_ui->regionTitle->setLevel(2);

    m_ui->printKeyActionGroup->setId(m_ui->newScreenshotButton, Settings::TakeNewScreenshot);
    m_ui->printKeyActionGroup->setId(m_ui->newWindowButton, Settings::StartNewInstance);
    m_ui->printKeyActionGroup->setId(m_ui->activateWindowButton, Settings::FocusWindow);

    //On Wayland  we can't programmatically raise and focus the window so we have to hide the option
    if (KWindowSystem::isPlatformWayland() || qstrcmp(qgetenv("XDG_SESSION_TYPE"), "wayland") == 0) {
        m_ui->formLayout->removeRow(m_ui->activateWindowButton);
    }
    //Workaround because KConfigDialogManager doesn't support QButtonGroup (Bug 409037)
    auto workaroundLabel = m_ui->kcfg_printKeyActionRunning;
    connect(workaroundLabel, &QLineEdit::textChanged, this, [this](const QString& text){
        auto button = m_ui->printKeyActionGroup->button(text.toInt());
        // We are missing a button on Wayland
        button ? button->setChecked(true) : m_ui->newScreenshotButton->setChecked(true);
    });
    connect(m_ui->printKeyActionGroup, qOverload<QAbstractButton *, bool>(&QButtonGroup::buttonToggled),
            workaroundLabel, [workaroundLabel, this] (QAbstractButton *button, bool checked) {
                if (checked) {
                    const int value = m_ui->printKeyActionGroup->id(button);
                    workaroundLabel->setText(QString::number(value));
                }
    });
    // /Workaround

}

GeneralOptionsPage::~GeneralOptionsPage() = default;
