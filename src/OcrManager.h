/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2025 Jhair Paris <dev@jhairparis.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "Config.h"

#ifdef HAVE_TESSERACT_OCR
#include <tesseract/baseapi.h>
#else
namespace tesseract
{
class TessBaseAPI;
}
#endif

#include <QImage>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QThread>
#include <QTimer>

#include <memory>

/**
 * @brief Worker class for OCR processing in background thread
 */
class OcrWorker : public QObject
{
    Q_OBJECT

public:
    explicit OcrWorker(QObject *parent = nullptr);

public Q_SLOTS:
    void processImage(const QImage &image, tesseract::TessBaseAPI *tesseract);

Q_SIGNALS:
    void imageProcessed(const QString &text, bool success);

private:
    QMutex m_mutex;
};

/**
 * This class uses Tesseract OCR engine to extract text from images.
 * It provides both synchronous and asynchronous text recognition capabilities.
 */
class OcrManager : public QObject
{
    Q_OBJECT

public:
    static constexpr int MAX_OCR_LANGUAGES = 4;
    static constexpr int MIN_OCR_LANGUAGES = 1;
    enum class OcrStatus {
        Ready = 0,
        Processing = 1,
        Error = 2
    };
    Q_ENUM(OcrStatus)

    explicit OcrManager(QObject *parent = nullptr);
    ~OcrManager() override;

    static OcrManager *instance();

    /**
     * @brief Check if OCR engine is available and properly initialized
     * @return true if OCR is available, false otherwise
     */
    bool isAvailable() const;

    /**
     * @brief Get the current OCR processing status
     * @return Current status of the OCR engine
     */
    OcrStatus status() const;

    /**
     * @brief Get a map of available languages with human-readable names
     * @return QMap where key is language code and value is display name
     */
    QMap<QString, QString> availableLanguagesWithNames() const;

    /**
     * @brief Set multiple languages for OCR processing
     * @param languageCodes List of language codes to use (e.g., ["eng", "spa", "fra"])
     */
    void setLanguagesByCode(const QStringList &languageCodes);

    /**
     * @brief Get the current language code
     * @return Current language code (e.g., "eng", "spa")
     */
    QString currentLanguageCode() const;

public Q_SLOTS:
    /**
     * @brief Extract text from an image asynchronously
     * @param image The image to process
     *
     * This method processes the image in a background thread and emits
     * textRecognized() signal when complete.
     */
    void recognizeText(const QImage &image);

    /**
     * @brief Extract text from an image using a temporary language selection
     * @param image The image to process
     * @param languageCode The one-off language code to use (e.g. "eng")
     *
     * The provided language is applied only for this recognition request and
     * does not persist the user's saved configuration.
     */
    void recognizeTextWithLanguage(const QImage &image, const QString &languageCode);

Q_SIGNALS:
    /**
     * @brief Emitted when text recognition is complete
     * @param text The recognized text
     * @param success true if recognition was successful
     */
    void textRecognized(const QString &text, bool success);

    /**
     * @brief Emitted when OCR status changes
     * @param status New status
     */
    void statusChanged(OcrStatus status);

private Q_SLOTS:
    void handleRecognitionComplete(const QString &text, bool success);

private:
    void initializeTesseract();
    void setStatus(OcrStatus status);
    bool setupTesseractLanguages(const QStringList &langCodes);
    void setupAvailableLanguages(const QString &tessdataPath);
    void loadSavedLanguageSetting();
    bool isLanguageAvailable(const QString &languageCode) const;
    QString tesseractLangName(const QString &tesseractCode) const;

    /**
     * @brief Validate, filter, and apply languages to Tesseract
     * @param languageCodes Languages to validate and apply
     * @return true if languages were successfully applied
     */
    bool validateAndApplyLanguages(const QStringList &languageCodes);
    void beginRecognition(const QImage &image);

    static OcrManager *s_instance;

#ifdef HAVE_TESSERACT_OCR
    tesseract::TessBaseAPI *m_tesseract;
    OcrWorker *m_worker;
#endif
    std::unique_ptr<QThread> m_workerThread;
    QTimer *m_timeoutTimer;

    OcrStatus m_status;
    QString m_currentLanguageCode;
    QStringList m_configuredLanguages;
    QStringList m_activeLanguages;
    bool m_shouldRestoreToConfigured;
    QStringList m_availableLanguages;
    QMap<QString, QString> m_languageNames;
    bool m_initialized;

private:
};