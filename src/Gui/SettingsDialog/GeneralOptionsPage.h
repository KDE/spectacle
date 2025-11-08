/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef GENERALOPTIONSPAGE_H
#define GENERALOPTIONSPAGE_H

#include <QScopedPointer>
#include <QWidget>

class Ui_GeneralOptions;
class OcrLanguageSelector;

class GeneralOptionsPage : public QWidget
{
    Q_OBJECT

public:
    explicit GeneralOptionsPage(QWidget *parent = nullptr);
    ~GeneralOptionsPage() override;

    void refreshOcrLanguageSettings(bool rebuildSelector = true);

    /**
     * @brief Get direct access to the OCR language selector widget
     * @return Pointer to the OcrLanguageSelector widget for direct manipulation
     */
    OcrLanguageSelector *ocrLanguageSelector() const
    {
        return m_ocrLanguageSelector;
    }

Q_SIGNALS:
    void ocrLanguageChanged();

private:
    QScopedPointer<Ui_GeneralOptions> m_ui;
    OcrLanguageSelector *m_ocrLanguageSelector;
};

#endif // GENERALOPTIONSPAGE_H
