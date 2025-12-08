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
#include <QSignalBlocker>
#include <QVBoxLayout>

using namespace Qt::Literals::StringLiterals;

OcrLanguageSelector::OcrLanguageSelector(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout(this))
    , m_ocrManager(OcrManager::instance())
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    setContentsMargins(0, 0, 0, 0);

    setupLanguageCheckboxes();

    connect(m_ocrManager, &OcrManager::statusChanged, this, &OcrLanguageSelector::onOcrManagerStatusChanged);

    if (m_ocrManager) {
        m_lastStatus = m_ocrManager->status();
        setProcessingState(m_lastStatus == OcrManager::OcrStatus::Processing);
        updateCheckboxEnabledStates(selectedLanguages().size());
    }
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
    QSignalBlocker blocker(this);

    for (QCheckBox *checkbox : m_languageCheckboxes) {
        const QString langCode = checkbox->property("languageCode").toString();
        QSignalBlocker checkboxBlocker(checkbox);
        checkbox->setChecked(languages.contains(langCode));
    }

    enforceSelectionLimits();
}

bool OcrLanguageSelector::isDefault() const
{
    const QStringList current = selectedLanguages();

    // Default state is exactly one language selected
    if (current.size() != 1) {
        return false;
    }

    QCheckBox *defaultCheckbox = findDefaultCheckbox();

    if (defaultCheckbox) {
        QString defaultLangCode = defaultCheckbox->property("languageCode").toString();
        return current.contains(defaultLangCode);
    }

    return false;
}

bool OcrLanguageSelector::hasChanges() const
{
    return selectedLanguages() != Settings::ocrLanguages();
}

void OcrLanguageSelector::applyDefaults()
{
    if (isProcessing()) {
        qCDebug(SPECTACLE_LOG) << "Ignoring OCR defaults while recognition is running";
        return;
    }

    if (m_languageCheckboxes.isEmpty()) {
        return;
    }

    QSignalBlocker blocker(this);

    QCheckBox *defaultCheckbox = findDefaultCheckbox();

    for (QCheckBox *checkbox : m_languageCheckboxes) {
        QSignalBlocker checkboxBlocker(checkbox);
        checkbox->setChecked(checkbox == defaultCheckbox);
    }

    const int selectedCount = defaultCheckbox ? 1 : 0;
    updateCheckboxEnabledStates(selectedCount);

    QStringList selected;
    if (defaultCheckbox) {
        selected.append(defaultCheckbox->property("languageCode").toString());
    }

    Q_EMIT selectedLanguagesChanged(selected);
}

void OcrLanguageSelector::refresh()
{
    if (isProcessing()) {
        qCDebug(SPECTACLE_LOG) << "Skipping OCR language refresh while recognition is running";
        return;
    }

    setupLanguageCheckboxes();
}

void OcrLanguageSelector::saveSettings()
{
    if (isProcessing()) {
        qCDebug(SPECTACLE_LOG) << "Ignoring OCR language save while recognition is running";
        return;
    }

    const QStringList selected = selectedLanguages();
    if (selected == Settings::ocrLanguages()) {
        return;
    }

    Settings::setOcrLanguages(selected);
}

void OcrLanguageSelector::updateWidgets()
{
    const QStringList savedLanguages = Settings::ocrLanguages();
    setSelectedLanguages(savedLanguages);
}

void OcrLanguageSelector::onLanguageCheckboxChanged()
{
    if (isProcessing()) {
        QSignalBlocker blocker(this);
        qCDebug(SPECTACLE_LOG) << "Discarding OCR language toggle while recognition is running";
        setSelectedLanguages(Settings::ocrLanguages());
        return;
    }

    enforceSelectionLimits();

    QStringList selected;
    selected.reserve(OcrManager::MAX_OCR_LANGUAGES);

    for (QCheckBox *checkbox : m_languageCheckboxes) {
        if (checkbox->isChecked()) {
            selected.append(checkbox->property("languageCode").toString());
        }
    }

    Q_EMIT selectedLanguagesChanged(selected);
}

void OcrLanguageSelector::onOcrManagerStatusChanged(OcrManager::OcrStatus status)
{
    const bool processing = status == OcrManager::OcrStatus::Processing;
    const bool processingChanged = processing != m_isProcessing;
    const bool statusChanged = status != m_lastStatus;

    setProcessingState(processing);

    if (!processing && (processingChanged || statusChanged)) {
        refresh();
    }

    if (!processing) {
        updateCheckboxEnabledStates(selectedLanguages().size());
    }

    m_lastStatus = status;
}

void OcrLanguageSelector::setupLanguageCheckboxes()
{
    if (isProcessing()) {
        qCDebug(SPECTACLE_LOG) << "Deferring OCR language checkbox rebuild while recognition is active";
        return;
    }

    while (QLayoutItem *item = m_layout->takeAt(0)) {
        if (auto widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    m_languageCheckboxes.clear();

    if (!m_ocrManager || !m_ocrManager->isAvailable()) {
        qCWarning(SPECTACLE_LOG) << "OCR is not available; language selector will remain empty.";
        return;
    }

    const QMap<QString, QString> availableLanguages = m_ocrManager->availableLanguagesWithNames();

    if (availableLanguages.isEmpty()) {
        qCWarning(SPECTACLE_LOG) << "No OCR language data available.";
        return;
    }

    for (auto it = availableLanguages.cbegin(); it != availableLanguages.cend(); ++it) {
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

    updateCheckboxEnabledStates(selectedLanguages().size());
}

void OcrLanguageSelector::enforceSelectionLimits()
{
    int selectedCount = 0;

    for (QCheckBox *checkbox : m_languageCheckboxes) {
        if (checkbox->isChecked()) {
            ++selectedCount;
        }
    }

    if (selectedCount > OcrManager::MAX_OCR_LANGUAGES) {
        for (int i = m_languageCheckboxes.size() - 1; i >= 0; --i) {
            QCheckBox *checkbox = m_languageCheckboxes[i];
            if (checkbox->isChecked()) {
                QSignalBlocker blocker(checkbox);
                checkbox->setChecked(false);
                --selectedCount;
                break;
            }
        }
    }

    updateCheckboxEnabledStates(selectedCount);

    if (selectedCount == 0 && !m_languageCheckboxes.isEmpty() && !isProcessing()) {
        applyDefaults();
    }
}

QCheckBox *OcrLanguageSelector::findDefaultCheckbox() const
{
    if (m_languageCheckboxes.isEmpty()) {
        return nullptr;
    }

    // Try English first
    for (QCheckBox *checkbox : m_languageCheckboxes) {
        if (checkbox->property("languageCode").toString() == u"eng"_s) {
            return checkbox;
        }
    }

    // Fallback to first available
    return m_languageCheckboxes.first();
}

bool OcrLanguageSelector::isProcessing() const
{
    return m_isProcessing;
}

void OcrLanguageSelector::setProcessingState(bool processing)
{
    if (m_isProcessing == processing) {
        return;
    }

    m_isProcessing = processing;
    setEnabled(!processing);

    if (processing) {
        for (QCheckBox *checkbox : m_languageCheckboxes) {
            checkbox->setEnabled(false);
        }
    }
}

void OcrLanguageSelector::updateCheckboxEnabledStates(int selectedCount)
{
    if (isProcessing()) {
        for (QCheckBox *checkbox : m_languageCheckboxes) {
            checkbox->setEnabled(false);
        }
        return;
    }

    const bool enableUnchecked = selectedCount < OcrManager::MAX_OCR_LANGUAGES;

    for (QCheckBox *checkbox : m_languageCheckboxes) {
        checkbox->setEnabled(checkbox->isChecked() || enableUnchecked);
    }
}

#include "moc_OcrLanguageSelector.cpp"