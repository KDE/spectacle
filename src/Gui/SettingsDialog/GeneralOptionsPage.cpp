/*
 *  SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "GeneralOptionsPage.h"

#include "settings.h"
#include "ui_GeneralOptions.h"

#include <KWindowSystem>

#include <QCheckBox>

GeneralOptionsPage::GeneralOptionsPage(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui_GeneralOptions)
{
    m_ui->setupUi(this);

    m_ui->runningTitle->setLevel(2);
    m_ui->regionTitle->setLevel(2);

    connect(m_ui->kcfg_copyImageToClipboard, &QCheckBox::stateChanged, this, &GeneralOptionsPage::updateAutomaticActions);
    connect(m_ui->kcfg_autoSaveImage, &QCheckBox::stateChanged, this, &GeneralOptionsPage::updateAutomaticActions);

    //On Wayland  we can't programmatically raise and focus the window so we have to hide the option
    if (KWindowSystem::isPlatformWayland() || qstrcmp(qgetenv("XDG_SESSION_TYPE").data(), "wayland") == 0) {
       delete m_ui->activateWindowButton;
    }
}

GeneralOptionsPage::~GeneralOptionsPage() = default;

void GeneralOptionsPage::updateAutomaticActions() {
    if (m_ui->kcfg_copyImageToClipboard->isChecked() && m_ui->kcfg_autoSaveImage->isChecked()) {
        m_ui->kcfg_copySaveLocation->setCheckState(Qt::CheckState::Unchecked);
        m_ui->kcfg_copySaveLocation->setCheckable(false);
        m_ui->kcfg_copySaveLocation->setDisabled(true);
    } else {
        m_ui->kcfg_copySaveLocation->setCheckable(true);
        m_ui->kcfg_copySaveLocation->setEnabled(true);
    }
}
