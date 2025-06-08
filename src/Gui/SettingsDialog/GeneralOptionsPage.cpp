/*
 *  SPDX-FileCopyrightText: 2025 Jhair Paris <dev@jhairparis.com>
 *  SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "GeneralOptionsPage.h"

#include "settings.h"
#include "ui_GeneralOptions.h"
#include "OcrManager.h"

#include <KWindowSystem>
#include <KLocalizedString>

#include <QCheckBox>
#include <QIcon>

GeneralOptionsPage::GeneralOptionsPage(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui_GeneralOptions)
{
    m_ui->setupUi(this);

    m_ui->ocrInfoIcon->setPixmap(QIcon::fromTheme(QStringLiteral("help-hint")).pixmap(16, 16));
    m_ui->ocrInfoIcon->setCursor(Qt::WhatsThisCursor);

    m_ui->runningTitle->setLevel(2);
    m_ui->regionTitle->setLevel(2);
    m_ui->ocrTitle->setLevel(2);

    setupOcrLanguageComboBox();

    connect(OcrManager::instance(), &OcrManager::statusChanged, this, &GeneralOptionsPage::refreshOcrLanguageSettings);

    //On Wayland  we can't programmatically raise and focus the window so we have to hide the option
    if (KWindowSystem::isPlatformWayland() || qstrcmp(qgetenv("XDG_SESSION_TYPE").constData(), "wayland") == 0) {
        m_ui->kcfg_printKeyRunningAction->removeItem(2);
    }
}

GeneralOptionsPage::~GeneralOptionsPage() = default;

void GeneralOptionsPage::setupOcrLanguageComboBox()
{
    OcrManager *ocrManager = OcrManager::instance();
    
    if (!ocrManager->isAvailable()) {
        m_ui->kcfg_ocrLanguage->setEnabled(false);
        m_ui->kcfg_ocrLanguage->addItem(i18n("OCR not available"));
        m_ui->ocrLanguageLabel->setVisible(false);
        m_ui->kcfg_ocrLanguage->setVisible(false);
        m_ui->ocrUnavailableWidget->setVisible(true);
        return;
    }
    
    const auto availableLanguages = ocrManager->availableLanguagesWithNames();
    
    if (availableLanguages.isEmpty()) {
        m_ui->kcfg_ocrLanguage->addItem(i18n("No languages found"));
        m_ui->kcfg_ocrLanguage->setEnabled(false);
        return;
    }
    
    m_ui->kcfg_ocrLanguage->clear();
    m_ui->ocrLanguageLabel->setVisible(true);
    m_ui->kcfg_ocrLanguage->setVisible(true);
    m_ui->ocrUnavailableWidget->setVisible(false);

    for (auto it = availableLanguages.constBegin(); it != availableLanguages.constEnd(); ++it) {
        m_ui->kcfg_ocrLanguage->addItem(it.value(), it.key());
    }
}

void GeneralOptionsPage::refreshOcrLanguageSettings()
{
    OcrManager *ocrManager = OcrManager::instance();
    
    if (!ocrManager->isAvailable()) {
        m_ui->ocrLanguageLabel->setVisible(false);
        m_ui->kcfg_ocrLanguage->setVisible(false);
        m_ui->ocrUnavailableWidget->setVisible(true);
        return;
    }
    
    const auto availableLanguages = ocrManager->availableLanguagesWithNames();
    
    if (availableLanguages.isEmpty()) {
        return;
    }
    
    m_ui->kcfg_ocrLanguage->clear();
    m_ui->kcfg_ocrLanguage->setEnabled(true);
    m_ui->ocrLanguageLabel->setVisible(true);
    m_ui->kcfg_ocrLanguage->setVisible(true);
    m_ui->ocrUnavailableWidget->setVisible(false);

    for (auto it = availableLanguages.constBegin(); it != availableLanguages.constEnd(); ++it) {
        m_ui->kcfg_ocrLanguage->addItem(it.value(), it.key());
    }
    
    const QString currentLanguage = Settings::ocrLanguage();
    
    for (int i = 0; i < m_ui->kcfg_ocrLanguage->count(); ++i) {
        if (m_ui->kcfg_ocrLanguage->itemData(i).toString() == currentLanguage) {
            m_ui->kcfg_ocrLanguage->setCurrentIndex(i);
            break;
        }
    }
}

#include "moc_GeneralOptionsPage.cpp"
