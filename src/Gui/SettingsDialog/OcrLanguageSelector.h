/*
 *  SPDX-FileCopyrightText: 2025 Jhair Paris <dev@jhairparis.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef OCRLANGUAGESELECTOR_H
#define OCRLANGUAGESELECTOR_H

#include <QCheckBox>
#include <QVBoxLayout>
#include <QWidget>

class OcrManager;

/**
 * @brief Specialized widget for OCR language selection with multi-language support
 *
 * This widget encapsulates all the logic for OCR language selection:
 * - Displays available languages as checkboxes (excluding 'osd')
 * - Enforces limits: minimum 1, maximum languages defined by OcrManager
 * - Handles defaults: English preferred, fallback to first available
 * - Follows KConfigDialog pattern: no auto-persistence, explicit save/update methods
 * - Updates dynamically when OCR manager state changes
 */
class OcrLanguageSelector : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QStringList selectedLanguages READ selectedLanguages WRITE setSelectedLanguages NOTIFY selectedLanguagesChanged USER true)
    Q_PROPERTY(bool isDefault READ isDefault NOTIFY selectedLanguagesChanged)
    Q_PROPERTY(bool hasChanges READ hasChanges NOTIFY selectedLanguagesChanged)

public:
    explicit OcrLanguageSelector(QWidget *parent = nullptr);
    ~OcrLanguageSelector() override;

    /**
     * @brief Get currently selected language codes
     * @return List of selected language codes (e.g., ["eng", "spa"])
     */
    QStringList selectedLanguages() const;

    /**
     * @brief Set selected languages
     * @param languages List of language codes to select
     */
    void setSelectedLanguages(const QStringList &languages);

    /**
     * @brief Check if current selection is the default state
     * @return true if selection represents default configuration
     */
    bool isDefault() const;

    /**
     * @brief Check if there are unsaved changes
     * @return true if current selection differs from saved configuration
     */
    bool hasChanges() const;

    /**
     * @brief Apply default language selection
     * Selects English if available, otherwise first available language
     */
    void applyDefaults();

    /**
     * @brief Refresh the widget when OCR manager state changes
     * Rebuilds checkboxes based on current available languages
     */
    void refresh();

    /**
     * @brief Save current selection to settings (called by KConfigDialog)
     * Follows KConfigDialog pattern for saving changes
     */
    void saveSettings();

    /**
     * @brief Update widget to reflect current settings (called by KConfigDialog)
     * Reloads settings when user cancels or dialog is reopened
     */
    void updateWidgets();

Q_SIGNALS:
    /**
     * @brief Emitted when language selection changes
     * @param languages New list of selected languages
     */
    void selectedLanguagesChanged(const QStringList &languages);

private Q_SLOTS:
    void onLanguageCheckboxChanged();
    void onOcrManagerStatusChanged();

private:
    void setupLanguageCheckboxes();
    void enforceSelectionLimits();
    void updateCheckboxEnabledStates();
    QString getDefaultLanguageCode() const;
    void blockSignalsAndSetChecked(QCheckBox *checkbox, bool checked);

    QVBoxLayout *m_layout;
    QList<QCheckBox *> m_languageCheckboxes;
    QMap<QString, QString> m_availableLanguages; // code -> display name
    bool m_blockSignals;

    OcrManager *m_ocrManager;
};

#endif // OCRLANGUAGESELECTOR_H