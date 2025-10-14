/*
 *  SPDX-FileCopyrightText: 2025 Jhair Paris <dev@jhairparis.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "OcrLanguageSelector.h"
#include "OcrManager.h"
#include "settings.h"
#include "spectacle_debug.h"

#include <KLocalizedString>

#include <QCheckBox>
#include <QVBoxLayout>

using namespace Qt::Literals::StringLiterals;

OcrLanguageSelector::OcrLanguageSelector(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout(this))
    , m_blockSignals(false)
    , m_ocrManager(OcrManager::instance())
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    setContentsMargins(0, 0, 0, 0);

    setupLanguageCheckboxes();

    connect(m_ocrManager, &OcrManager::statusChanged, this, &OcrLanguageSelector::onOcrManagerStatusChanged);
}

OcrLanguageSelector::~OcrLanguageSelector() = default;

QStringList OcrLanguageSelector::selectedLanguages() const
{
    QStringList result;
    for (QCheckBox *checkbox : m_languageCheckboxes) {
        if (checkbox->isChecked()) {
            result.append(checkbox->property("languageCode").toString());
        }
    }
    return result;
}

void OcrLanguageSelector::setSelectedLanguages(const QStringList &languages)
{
    m_blockSignals = true;

    for (QCheckBox *checkbox : m_languageCheckboxes) {
        const QString langCode = checkbox->property("languageCode").toString();
        checkbox->setChecked(languages.contains(langCode));
    }

    m_blockSignals = false;

    enforceSelectionLimits();
}

bool OcrLanguageSelector::isDefault() const
{
    const QStringList current = selectedLanguages();

    // Default state is exactly one language selected
    if (current.size() != 1) {
        return false;
    }

    // Check if it's English (preferred default)
    for (const QCheckBox *checkbox : m_languageCheckboxes) {
        if (checkbox->property("languageCode").toString() == u"eng"_s) {
            // English is available, so default is English
            return current.contains(u"eng"_s);
        }
    }

    // English not available, default is the first available language
    if (!m_languageCheckboxes.isEmpty()) {
        QString firstLangCode = m_languageCheckboxes.first()->property("languageCode").toString();
        return current.contains(firstLangCode);
    }

    return false;
}

bool OcrLanguageSelector::hasChanges() const
{
    return selectedLanguages() != Settings::ocrLanguages();
}

void OcrLanguageSelector::applyDefaults()
{
    if (!m_languageCheckboxes.isEmpty()) {
        m_blockSignals = true;

        for (QCheckBox *checkbox : m_languageCheckboxes) {
            checkbox->setChecked(false);
        }

        // Try to select English first
        bool foundDefault = false;
        for (QCheckBox *checkbox : m_languageCheckboxes) {
            if (checkbox->property("languageCode").toString() == u"eng"_s) {
                checkbox->setChecked(true);
                foundDefault = true;
                break;
            }
        }

        // If English not available, select first language
        if (!foundDefault) {
            m_languageCheckboxes.first()->setChecked(true);
        }

        m_blockSignals = false;

        const QStringList selected = selectedLanguages();
        Settings::setOcrLanguages(selected);

        // Emit signal to notify changes
        Q_EMIT selectedLanguagesChanged(selected);
    }
}

void OcrLanguageSelector::refresh()
{
    setupLanguageCheckboxes();
}

void OcrLanguageSelector::saveSettings()
{
    const QStringList selected = selectedLanguages();
    Settings::setOcrLanguages(selected);
}

void OcrLanguageSelector::updateWidgets()
{
    const QStringList savedLanguages = Settings::ocrLanguages();
    setSelectedLanguages(savedLanguages);
}

void OcrLanguageSelector::onLanguageCheckboxChanged()
{
    if (m_blockSignals) {
        return;
    }

    enforceSelectionLimits();

    const QStringList selected = selectedLanguages();
    Q_EMIT selectedLanguagesChanged(selected);
}

void OcrLanguageSelector::onOcrManagerStatusChanged()
{
    refresh();
}

void OcrLanguageSelector::setupLanguageCheckboxes()
{
    while (QLayoutItem *item = m_layout->takeAt(0)) {
        if (auto widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    m_languageCheckboxes.clear();
    m_availableLanguages.clear();

    if (!m_ocrManager || !m_ocrManager->isAvailable()) {
        qCWarning(SPECTACLE_LOG) << "OCR is not available; language selector will remain empty.";
        return;
    }

    m_availableLanguages = m_ocrManager->availableLanguagesWithNames();

    if (m_availableLanguages.isEmpty()) {
        qCWarning(SPECTACLE_LOG) << "No OCR language data available.";
        return;
    }

    for (auto it = m_availableLanguages.cbegin(); it != m_availableLanguages.cend(); ++it) {
        const QString &langCode = it.key();
        if (langCode == u"osd"_s) {
            continue;
        }

        QCheckBox *checkbox = new QCheckBox(it.value(), this);
        checkbox->setProperty("languageCode", langCode);
        connect(checkbox, &QCheckBox::toggled, this, &OcrLanguageSelector::onLanguageCheckboxChanged);
        m_layout->addWidget(checkbox);
        m_languageCheckboxes.append(checkbox);
    }

    if (m_layout->count() > 0) {
        m_layout->addStretch();
    }

    const QStringList savedLanguages = Settings::ocrLanguages();
    setSelectedLanguages(savedLanguages);

    if (savedLanguages.isEmpty() && !m_languageCheckboxes.isEmpty()) {
        applyDefaults();
    }
}

void OcrLanguageSelector::enforceSelectionLimits()
{
    const QStringList selected = selectedLanguages();
    const int count = selected.size();

    if (count > OcrManager::MAX_OCR_LANGUAGES) { // Max languages for performance
        for (int i = m_languageCheckboxes.size() - 1; i >= 0; --i) {
            QCheckBox *checkbox = m_languageCheckboxes[i];
            if (checkbox->isChecked()) {
                blockSignalsAndSetChecked(checkbox, false);
                break;
            }
        }
    }

    updateCheckboxEnabledStates();

    if (selectedLanguages().size() == 0 && !m_languageCheckboxes.isEmpty()) {
        applyDefaults();
    }
}

QString OcrLanguageSelector::getDefaultLanguageCode() const
{
    if (m_languageCheckboxes.isEmpty()) {
        return QString();
    }

    // Try English first
    for (const QCheckBox *checkbox : m_languageCheckboxes) {
        if (checkbox->property("languageCode").toString() == u"eng"_s) {
            return u"eng"_s;
        }
    }

    // Fallback to first available
    return m_languageCheckboxes.first()->property("languageCode").toString();
}

void OcrLanguageSelector::updateCheckboxEnabledStates()
{
    const QStringList selected = selectedLanguages();
    const int count = selected.size();

    // If we have max languages selected, disable all unchecked checkboxes
    // If we have less than max, enable all checkboxes
    for (QCheckBox *checkbox : m_languageCheckboxes) {
        if (checkbox->isChecked()) {
            checkbox->setEnabled(true);
        } else {
            checkbox->setEnabled(count < OcrManager::MAX_OCR_LANGUAGES);
        }
    }
}

void OcrLanguageSelector::blockSignalsAndSetChecked(QCheckBox *checkbox, bool checked)
{
    m_blockSignals = true;
    checkbox->setChecked(checked);
    m_blockSignals = false;
}

#include "moc_OcrLanguageSelector.cpp"