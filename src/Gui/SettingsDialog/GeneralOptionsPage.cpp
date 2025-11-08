/*
 *  SPDX-FileCopyrightText: 2025 Jhair Paris <dev@jhairparis.com>
 *  SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "GeneralOptionsPage.h"

#include "OcrLanguageSelector.h"
#include "OcrManager.h"
#include "ui_GeneralOptions.h"

#include <KLocalizedString>
#include <KWindowSystem>

#include <QIcon>

using namespace Qt::Literals::StringLiterals;

GeneralOptionsPage::GeneralOptionsPage(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui_GeneralOptions)
    , m_ocrLanguageSelector(new OcrLanguageSelector(this))
{
    m_ui->setupUi(this);

    m_ui->ocrInfoIcon->setPixmap(QIcon::fromTheme(QStringLiteral("help-hint")).pixmap(16, 16));
    m_ui->ocrInfoIcon->setCursor(Qt::WhatsThisCursor);

    m_ui->runningTitle->setLevel(2);
    m_ui->regionTitle->setLevel(2);
    m_ui->ocrTitle->setLevel(2);

    m_ui->ocrLanguageScrollArea->setWidget(m_ocrLanguageSelector);
    m_ui->ocrLanguageScrollArea->setWidgetResizable(true);

    connect(m_ocrLanguageSelector, &OcrLanguageSelector::selectedLanguagesChanged, this, &GeneralOptionsPage::ocrLanguageChanged);

    refreshOcrLanguageSettings();

    //On Wayland  we can't programmatically raise and focus the window so we have to hide the option
    if (KWindowSystem::isPlatformWayland() || qstrcmp(qgetenv("XDG_SESSION_TYPE").constData(), "wayland") == 0) {
        m_ui->kcfg_printKeyRunningAction->removeItem(2);
    }
}

GeneralOptionsPage::~GeneralOptionsPage() = default;

void GeneralOptionsPage::refreshOcrLanguageSettings(bool rebuildSelector)
{
    OcrManager *ocrManager = OcrManager::instance();
    
    if (!ocrManager->isAvailable()) {
        m_ui->ocrLanguageLabel->setVisible(false);
        m_ui->ocrLanguageScrollArea->setVisible(false);
        m_ui->ocrUnavailableWidget->setVisible(true);
    } else {
        m_ui->ocrLanguageLabel->setVisible(true);
        m_ui->ocrLanguageScrollArea->setVisible(true);
        m_ui->ocrUnavailableWidget->setVisible(false);

        if (rebuildSelector) {
            m_ocrLanguageSelector->refresh();
        }
    }
}

#include "moc_GeneralOptionsPage.cpp"
