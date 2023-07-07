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

    //On Wayland  we can't programmatically raise and focus the window so we have to hide the option
    if (KWindowSystem::isPlatformWayland() || qstrcmp(qgetenv("XDG_SESSION_TYPE").constData(), "wayland") == 0) {
        m_ui->kcfg_printKeyActionRunning->removeItem(2);
    }
}

GeneralOptionsPage::~GeneralOptionsPage() = default;

#include "moc_GeneralOptionsPage.cpp"
