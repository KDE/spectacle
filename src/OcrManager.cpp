/*
 *  SPDX-FileCopyrightText: 2025 Jhair Paris <dev@jhairparis.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "OcrManager.h"
#include "settings.h"
#include "spectacle_debug.h"

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QStringList>
#include <QThread>

#include <KLocalizedString>

#include <algorithm>
#include <memory>

using namespace Qt::StringLiterals;

OcrManager *OcrManager::s_instance = nullptr;

OcrManager::OcrManager(QObject *parent)
    : QObject(parent)
    , m_tesseract(nullptr)
    , m_runtimeApi(nullptr)
    , m_worker(nullptr)
    , m_workerThread(std::make_unique<QThread>())
    , m_timeoutTimer(new QTimer(this))
    , m_status(OcrStatus::Ready)
    , m_currentLanguageCode() // Current language code ("eng+spa")
    , m_configuredLanguages() // Languages from Settings (persistent)
    , m_activeLanguages()
    , m_shouldRestoreToConfigured(false) // Flag to restore after temp language use
    , m_initialized(false)
{
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setInterval(30000);

    connect(m_timeoutTimer, &QTimer::timeout, this, [this]() {
        qCWarning(SPECTACLE_LOG) << "OCR recognition timed out";
        setStatus(OcrStatus::Error);
    });

    m_worker = new OcrWorker();
    m_worker->moveToThread(m_workerThread.get());
    connect(m_worker, &OcrWorker::imageProcessed, this, &OcrManager::handleRecognitionComplete);
    m_workerThread->start();

    connect(Settings::self(), &Settings::ocrLanguagesChanged, this, [this]() {
        if (m_configSyncSuspended) {
            return;
        }
        const QStringList newLanguages = Settings::ocrLanguages();
        const QString combinedLanguages = newLanguages.join(u"+"_s);
        if (combinedLanguages != m_currentLanguageCode) {
            setLanguagesByCode(newLanguages);
        }
    });

    QTimer::singleShot(0, this, &OcrManager::initializeTesseract);
}

OcrManager::~OcrManager()
{
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        if (!m_workerThread->wait(3000)) {
            qCWarning(SPECTACLE_LOG) << "Worker thread did not stop in time, terminating";
            m_workerThread->terminate();
            m_workerThread->wait();
        }
    }
    if (m_worker) {
        delete m_worker;
        m_worker = nullptr;
    }
    if (m_runtimeApi && m_tesseract) {
        m_runtimeApi->end(m_tesseract);
        m_runtimeApi->dispose(m_tesseract);
        m_tesseract = nullptr;
    }
}

OcrManager *OcrManager::instance()
{
    if (!s_instance) {
        s_instance = new OcrManager(qApp);
    }
    return s_instance;
}

bool OcrManager::isAvailable() const
{
    return m_initialized && m_tesseract != nullptr && m_runtimeApi != nullptr;
}

OcrManager::OcrStatus OcrManager::status() const
{
    return m_status;
}

QMap<QString, QString> OcrManager::availableLanguagesWithNames() const
{
    QMap<QString, QString> result;
    for (const QString &langCode : m_availableLanguages) {
        result[langCode] = m_languageNames.value(langCode, langCode);
    }
    return result;
}

void OcrManager::setLanguagesByCode(const QStringList &languageCodes)
{
    if (languageCodes.isEmpty()) {
        qCWarning(SPECTACLE_LOG) << "No OCR languages specified";
        return;
    }

    if (validateAndApplyLanguages(languageCodes)) {
        m_configuredLanguages = m_activeLanguages;
        Settings::setOcrLanguages(m_activeLanguages);
        Settings::self()->save();
        qCDebug(SPECTACLE_LOG) << "OCR languages successfully changed to:" << m_currentLanguageCode;
    } else {
        qCWarning(SPECTACLE_LOG) << "Failed to set OCR languages";
    }
}

QString OcrManager::currentLanguageCode() const
{
    return m_currentLanguageCode;
}

void OcrManager::setConfigSyncSuspended(bool suspended)
{
    if (m_configSyncSuspended == suspended) {
        return;
    }

    m_configSyncSuspended = suspended;

    // On resume, apply any changes made to Settings
    if (!m_configSyncSuspended) {
        const QStringList settingsLanguages = Settings::ocrLanguages();
        if (settingsLanguages != m_configuredLanguages) {
            setLanguagesByCode(settingsLanguages);
        }
    }
}

bool OcrManager::isConfigSyncSuspended() const
{
    return m_configSyncSuspended;
}

void OcrManager::recognizeText(const QImage &image)
{
    if (!isAvailable()) {
        qCWarning(SPECTACLE_LOG) << "Cannot start OCR: engine is not available";
        Q_EMIT textRecognized(QString(), QStringList(), false);
        return;
    }

    if (m_status == OcrStatus::Processing) {
        qCWarning(SPECTACLE_LOG) << "Cannot start OCR: text extraction already running";
        Q_EMIT textRecognized(QString(), QStringList(), false);
        return;
    }

    if (image.isNull() || image.size().isEmpty()) {
        qCWarning(SPECTACLE_LOG) << "Cannot start OCR: invalid image provided";
        Q_EMIT textRecognized(QString(), QStringList(), false);
        return;
    }

    // Ensure configured languages are active
    if (m_configuredLanguages.isEmpty() || m_activeLanguages != m_configuredLanguages) {
        if (!validateAndApplyLanguages(m_configuredLanguages)) {
            qCWarning(SPECTACLE_LOG) << "Cannot start OCR: failed to activate configured languages";
            Q_EMIT textRecognized(QString(), QStringList(), false);
            return;
        }
    }

    beginRecognition(image);
}

void OcrManager::recognizeTextWithLanguage(const QImage &image, const QString &languageCode)
{
    if (languageCode.isEmpty()) {
        recognizeText(image);
        return;
    }

    if (!isAvailable()) {
        qCWarning(SPECTACLE_LOG) << "Cannot start OCR with language" << languageCode << ": engine is not available";
        Q_EMIT textRecognized(QString(), QStringList(), false);
        return;
    }

    if (m_status == OcrStatus::Processing) {
        qCWarning(SPECTACLE_LOG) << "Cannot start OCR with language" << languageCode << ": text extraction already running";
        Q_EMIT textRecognized(QString(), QStringList(), false);
        return;
    }

    if (image.isNull() || image.size().isEmpty()) {
        qCWarning(SPECTACLE_LOG) << "Cannot start OCR with language" << languageCode << ": invalid image provided";
        Q_EMIT textRecognized(QString(), QStringList(), false);
        return;
    }

    const QStringList tempLanguages{languageCode};
    if (!validateAndApplyLanguages(tempLanguages)) {
        qCWarning(SPECTACLE_LOG) << "Cannot start OCR with language" << languageCode << ": failed to activate language";
        Q_EMIT textRecognized(QString(), QStringList(), false);
        return;
    }

    // Store that we need to restore after recognition
    m_shouldRestoreToConfigured = (m_activeLanguages != m_configuredLanguages);

    beginRecognition(image);
}

void OcrManager::handleRecognitionComplete(const QString &text, bool success)
{
    m_timeoutTimer->stop();

    if (success) {
        setStatus(OcrStatus::Ready);

        if (!text.isEmpty()) {
            QApplication::clipboard()->setText(text);
        }

        Q_EMIT textRecognized(text, m_activeLanguages, true);
        qCDebug(SPECTACLE_LOG) << "OCR recognition completed successfully";
    } else {
        setStatus(OcrStatus::Error);
        Q_EMIT textRecognized(QString(), QStringList(), false);
        qCWarning(SPECTACLE_LOG) << "OCR recognition failed";
    }

    // Restore configured languages if we used temporary ones
    if (m_shouldRestoreToConfigured && !m_configuredLanguages.isEmpty()) {
        validateAndApplyLanguages(m_configuredLanguages);
        m_shouldRestoreToConfigured = false;
    }
}

bool OcrManager::validateAndApplyLanguages(const QStringList &languageCodes)
{
    if (languageCodes.isEmpty()) {
        qCWarning(SPECTACLE_LOG) << "No OCR languages provided";
        return false;
    }

    QStringList validLanguages;
    for (const QString &lang : languageCodes) {
        if (lang == u"osd"_s) {
            qCDebug(SPECTACLE_LOG) << "Skipping 'osd' language";
            continue;
        }

        if (!isLanguageAvailable(lang)) {
            qCWarning(SPECTACLE_LOG) << "OCR language not available:" << lang;
            continue;
        }

        if (!validLanguages.contains(lang)) {
            validLanguages.append(lang);
        }
    }

    if (validLanguages.isEmpty()) {
        qCWarning(SPECTACLE_LOG) << "No valid OCR languages after filtering";
        return false;
    }

    if (validLanguages.size() > MAX_OCR_LANGUAGES) {
        validLanguages = validLanguages.mid(0, MAX_OCR_LANGUAGES);
        qCInfo(SPECTACLE_LOG) << "Limited to" << MAX_OCR_LANGUAGES << "languages:" << validLanguages;
    }

    const QString combinedLanguages = validLanguages.join(u"+"_s);

    if (m_currentLanguageCode == combinedLanguages && !m_activeLanguages.isEmpty()) {
        qCDebug(SPECTACLE_LOG) << "Languages already active, no change needed";
        return true;
    }

    if (!setupTesseractLanguages(validLanguages)) {
        qCWarning(SPECTACLE_LOG) << "Failed to apply OCR languages:" << combinedLanguages;
        return false;
    }

    m_activeLanguages = validLanguages;
    m_currentLanguageCode = combinedLanguages;

    qCDebug(SPECTACLE_LOG) << "OCR languages applied:" << combinedLanguages;
    return true;
}

void OcrManager::beginRecognition(const QImage &image)
{
    setStatus(OcrStatus::Processing);
    m_timeoutTimer->start();

    QMetaObject::invokeMethod(
        m_worker,
        [worker = m_worker, image, tesseract = m_tesseract, runtimeApi = m_runtimeApi]() {
            worker->processImage(image, tesseract, runtimeApi);
        },
        Qt::QueuedConnection);
}

void OcrManager::initializeTesseract()
{
    auto cleanupTesseract = [this]() {
        if (m_runtimeApi && m_tesseract) {
            m_runtimeApi->end(m_tesseract);
            m_runtimeApi->dispose(m_tesseract);
            m_tesseract = nullptr;
        }
    };

    auto &loader = TesseractRuntimeLoader::instance();
    if (!loader.ensureLoaded()) {
        qCWarning(SPECTACLE_LOG) << "Tesseract runtime library not available";
        setStatus(OcrStatus::Error);
        return;
    }

    m_runtimeApi = loader.api();
    if (!m_runtimeApi) {
        qCWarning(SPECTACLE_LOG) << "Missing Tesseract runtime API";
        setStatus(OcrStatus::Error);
        return;
    }

    try {
        m_tesseract = m_runtimeApi->create();
        if (!m_tesseract) {
            qCWarning(SPECTACLE_LOG) << "Failed to allocate Tesseract API";
            setStatus(OcrStatus::Error);
            return;
        }

        if (m_runtimeApi->init3(m_tesseract, nullptr, nullptr) != 0) {
            qCWarning(SPECTACLE_LOG) << "Failed to initialize Tesseract OCR engine";
            setStatus(OcrStatus::Error);
            cleanupTesseract();
            return;
        }

        const char *datapath = m_runtimeApi->datapath(m_tesseract);
        QString tessdataPath = datapath ? QString::fromUtf8(datapath) : QString();
        if (tessdataPath.isEmpty()) {
            qCWarning(SPECTACLE_LOG) << "Tesseract datapath is empty";
            setStatus(OcrStatus::Error);
            cleanupTesseract();
            return;
        }
        qCDebug(SPECTACLE_LOG) << "Using tessdata path:" << tessdataPath;

        setupAvailableLanguages(tessdataPath);

        if (m_availableLanguages.isEmpty()) {
            qCWarning(SPECTACLE_LOG) << "No language data files found in tessdata directory";
            setStatus(OcrStatus::Error);
            cleanupTesseract();
            return;
        }

        m_runtimeApi->end(m_tesseract);

        QStringList configLanguages = Settings::ocrLanguages();
        QStringList initLanguages;

        // Use configured languages if valid, otherwise fallback to first available
        for (const QString &lang : configLanguages) {
            if (!lang.isEmpty() && m_availableLanguages.contains(lang) && lang != u"osd"_s) {
                initLanguages.append(lang);
            }
        }

        if (initLanguages.isEmpty()) {
            auto it = std::find_if(m_availableLanguages.begin(), m_availableLanguages.end(), [](const QString &lang) {
                return lang != u"osd"_s;
            });

            if (it != m_availableLanguages.end()) {
                initLanguages.append(*it);
            } else {
                qCCritical(SPECTACLE_LOG) << "No fallback language available (only osd present)";
                setStatus(OcrStatus::Error);
                cleanupTesseract();
                return;
            }
        }

        const QString combinedInitLanguages = initLanguages.join(u"+"_s);
        qCDebug(SPECTACLE_LOG) << "Initializing Tesseract with languages:" << combinedInitLanguages;

        if (m_runtimeApi->init3(m_tesseract, nullptr, combinedInitLanguages.toUtf8().constData()) != 0) {
            qCWarning(SPECTACLE_LOG) << "Failed to initialize Tesseract with languages:" << combinedInitLanguages;
            setStatus(OcrStatus::Error);
            cleanupTesseract();
            return;
        }

        m_currentLanguageCode = combinedInitLanguages;
        m_runtimeApi->setPageSegMode(m_tesseract, PSM_AUTO);

        m_initialized = true;
        setStatus(OcrStatus::Ready);
        qCDebug(SPECTACLE_LOG) << "Tesseract OCR engine initialized successfully with languages:" << combinedInitLanguages;

        loadSavedLanguageSetting();
    } catch (const std::exception &e) {
        qCWarning(SPECTACLE_LOG) << "Exception during Tesseract initialization:" << e.what();
        setStatus(OcrStatus::Error);
        cleanupTesseract();
    }
}

void OcrManager::loadSavedLanguageSetting()
{
    if (!isAvailable()) {
        qCDebug(SPECTACLE_LOG) << "OCR not available, skipping language loading";
        return;
    }

    QStringList savedLanguages = Settings::ocrLanguages();
    qCDebug(SPECTACLE_LOG) << "Loaded OCR languages setting from config:" << savedLanguages;
    qCDebug(SPECTACLE_LOG) << "Current OCR language code:" << m_currentLanguageCode;
    qCDebug(SPECTACLE_LOG) << "Available languages:" << m_availableLanguages;

    QStringList validLanguages;
    for (const QString &lang : savedLanguages) {
        if (lang != u"osd"_s && isLanguageAvailable(lang)) {
            validLanguages.append(lang);
        }
    }

    if (validLanguages.isEmpty()) {
        // Find first valid language as fallback
        auto it = std::find_if(m_availableLanguages.begin(), m_availableLanguages.end(), [](const QString &lang) {
            return lang != u"osd"_s;
        });
        if (it != m_availableLanguages.end()) {
            validLanguages.append(*it);
        } else {
            qCWarning(SPECTACLE_LOG) << "No usable languages available (only osd present), cannot set default";
            return;
        }
        qCDebug(SPECTACLE_LOG) << "No valid saved languages, using default:" << validLanguages;
        Settings::setOcrLanguages(validLanguages);
        Settings::self()->save();
    }

    m_configuredLanguages = validLanguages;

    const QString combinedLanguages = validLanguages.join(u"+"_s);
    if (combinedLanguages != m_currentLanguageCode) {
        qCDebug(SPECTACLE_LOG) << "Loading OCR languages setting:" << validLanguages;
        validateAndApplyLanguages(validLanguages);
    } else {
        qCDebug(SPECTACLE_LOG) << "OCR languages already set to:" << combinedLanguages;
        m_activeLanguages = validLanguages;
    }
}

void OcrManager::setStatus(OcrStatus status)
{
    if (m_status == status) {
        return;
    }

    m_status = status;
    Q_EMIT statusChanged(status);
}

bool OcrManager::isLanguageAvailable(const QString &languageCode) const
{
    return m_availableLanguages.contains(languageCode);
}

bool OcrManager::setupTesseractLanguages(const QStringList &langCodes)
{
    if (!m_tesseract || !m_runtimeApi || langCodes.isEmpty()) {
        return false;
    }

    const char *datapath = m_runtimeApi->datapath(m_tesseract);
    QString tessdataPath = datapath ? QString::fromUtf8(datapath) : QString();

    if (tessdataPath.isEmpty()) {
        qCWarning(SPECTACLE_LOG) << "Tessdata path not found";
        return false;
    }

    for (const QString &langCode : langCodes) {
        const QString langFile = QDir(tessdataPath).filePath(langCode + u".traineddata"_s);
        if (!QFile::exists(langFile)) {
            qCWarning(SPECTACLE_LOG) << "Language file not found:" << langFile;
            return false;
        }
    }

    try {
        m_runtimeApi->end(m_tesseract);

        const QString combinedLangs = langCodes.join(u"+"_s);

        if (m_runtimeApi->init3(m_tesseract, nullptr, combinedLangs.toUtf8().constData()) != 0) {
            // Fallback to first available language
            QString fallbackLang;
            if (!m_availableLanguages.isEmpty()) {
                auto it = std::find_if(m_availableLanguages.begin(), m_availableLanguages.end(), [](const QString &lang) {
                    return lang != u"osd"_s;
                });
                if (it != m_availableLanguages.end()) {
                    fallbackLang = *it;
                }
            }

            if (fallbackLang.isEmpty() || m_runtimeApi->init3(m_tesseract, nullptr, fallbackLang.toUtf8().constData()) != 0) {
                qCWarning(SPECTACLE_LOG) << "Failed to initialize Tesseract with languages:" << combinedLangs << "and fallback:" << fallbackLang;
                return false;
            }

            m_activeLanguages = QStringList{fallbackLang};
            m_currentLanguageCode = fallbackLang;
        }

        m_runtimeApi->setPageSegMode(m_tesseract, PSM_AUTO);
        return true;
    } catch (const std::exception &e) {
        qCWarning(SPECTACLE_LOG) << "Exception while setting up Tesseract languages:" << e.what();
        return false;
    }
}

void OcrManager::setupAvailableLanguages(const QString &tessdataPath)
{
    m_availableLanguages.clear();
    m_languageNames.clear();

    if (!m_tesseract) {
        qCWarning(SPECTACLE_LOG) << "Cannot enumerate OCR languages: Tesseract not initialized";
        return;
    }

    QStringList detectedLanguages;

    if (!m_runtimeApi) {
        qCWarning(SPECTACLE_LOG) << "Cannot enumerate OCR languages: Runtime API not available";
        return;
    }

    char **languages = m_runtimeApi->getAvailableLanguagesAsVector(m_tesseract);
    if (!languages) {
        qCWarning(SPECTACLE_LOG) << "Tesseract API returned no languages";
        return;
    }

    int count = 0;
    for (char **entry = languages; *entry != nullptr; ++entry) {
        count++;
    }

    detectedLanguages.reserve(count);
    for (char **entry = languages; *entry != nullptr; ++entry) {
        const QString langCode = QString::fromUtf8(*entry);
        if (langCode.isEmpty()) {
            continue;
        }

        if (!tessdataPath.isEmpty()) {
            const QString trainedDataPath = QDir(tessdataPath).filePath(langCode + u".traineddata"_s);
            if (!QFile::exists(trainedDataPath)) {
                qCDebug(SPECTACLE_LOG) << "Skipping OCR language" << langCode << "- missing traineddata at" << trainedDataPath;
                continue;
            }
        }

        if (!detectedLanguages.contains(langCode)) {
            detectedLanguages.append(langCode);
        }
    }

    m_runtimeApi->deleteTextArray(languages);

    std::sort(detectedLanguages.begin(), detectedLanguages.end());
    m_availableLanguages = detectedLanguages;

    for (const QString &langCode : std::as_const(m_availableLanguages)) {
        if (langCode == u"osd"_s) {
            m_languageNames.insert(langCode, i18nc("@item:inlistbox", "Orientation and Script Detection"));
            continue;
        }

        const QString displayName = tesseractLangName(langCode);
        m_languageNames.insert(langCode, displayName);
    }

    qCDebug(SPECTACLE_LOG) << "Detected OCR languages:" << m_availableLanguages;
}

QString OcrManager::tesseractLangName(const QString &tesseractCode) const
{
    static const QMap<QString, QString> tesseractToIsoMap = {
        {u"afr"_s, u"af"_s},        {u"ara"_s, u"ar"_s},      {u"aze"_s, u"az"_s},     {u"aze_cyrl"_s, u"az"_s}, {u"bel"_s, u"be"_s},
        {u"ben"_s, u"bn"_s},        {u"bul"_s, u"bg"_s},      {u"cat"_s, u"ca"_s},     {u"ces"_s, u"cs"_s},      {u"chi_sim"_s, u"zh_CN"_s},
        {u"chi_tra"_s, u"zh_TW"_s}, {u"cym"_s, u"cy"_s},      {u"dan"_s, u"da"_s},     {u"dan_frak"_s, u"da"_s}, {u"deu"_s, u"de"_s},
        {u"deu_frak"_s, u"de"_s},   {u"deu_latf"_s, u"de"_s}, {u"ell"_s, u"el"_s},     {u"eng"_s, u"en"_s},      {u"epo"_s, u"eo"_s},
        {u"est"_s, u"et"_s},        {u"eus"_s, u"eu"_s},      {u"fas"_s, u"fa"_s},     {u"fin"_s, u"fi"_s},      {u"fra"_s, u"fr"_s},
        {u"frk"_s, u"de"_s},        {u"gla"_s, u"gd"_s},      {u"gle"_s, u"ga"_s},     {u"glg"_s, u"gl"_s},      {u"heb"_s, u"he"_s},
        {u"hin"_s, u"hi"_s},        {u"hrv"_s, u"hr"_s},      {u"hun"_s, u"hu"_s},     {u"ind"_s, u"id"_s},      {u"isl"_s, u"is"_s},
        {u"ita"_s, u"it"_s},        {u"ita_old"_s, u"it"_s},  {u"jpn"_s, u"ja"_s},     {u"kor"_s, u"ko"_s},      {u"kor_vert"_s, u"ko"_s},
        {u"lav"_s, u"lv"_s},        {u"lit"_s, u"lt"_s},      {u"nld"_s, u"nl"_s},     {u"nor"_s, u"no"_s},      {u"pol"_s, u"pl"_s},
        {u"por"_s, u"pt"_s},        {u"ron"_s, u"ro"_s},      {u"rus"_s, u"ru"_s},     {u"slk"_s, u"sk"_s},      {u"slk_frak"_s, u"sk"_s},
        {u"slv"_s, u"sl"_s},        {u"spa"_s, u"es"_s},      {u"spa_old"_s, u"es"_s}, {u"srp"_s, u"sr"_s},      {u"srp_latn"_s, u"sr"_s},
        {u"swe"_s, u"sv"_s},        {u"tur"_s, u"tr"_s},      {u"ukr"_s, u"uk"_s},     {u"vie"_s, u"vi"_s},      {u"amh"_s, u"am"_s},
        {u"asm"_s, u"as"_s},        {u"bod"_s, u"bo"_s},      {u"dzo"_s, u"dz"_s},     {u"guj"_s, u"gu"_s},      {u"kan"_s, u"kn"_s},
        {u"kat"_s, u"ka"_s},        {u"kat_old"_s, u"ka"_s},  {u"kaz"_s, u"kk"_s},     {u"khm"_s, u"km"_s},      {u"kir"_s, u"ky"_s},
        {u"lao"_s, u"lo"_s},        {u"mal"_s, u"ml"_s},      {u"mar"_s, u"mr"_s},     {u"mya"_s, u"my"_s},      {u"nep"_s, u"ne"_s},
        {u"ori"_s, u"or"_s},        {u"pan"_s, u"pa"_s},      {u"sin"_s, u"si"_s},     {u"tam"_s, u"ta"_s},      {u"tel"_s, u"te"_s},
        {u"tha"_s, u"th"_s},        {u"urd"_s, u"ur"_s},      {u"bos"_s, u"bs"_s},     {u"bre"_s, u"br"_s},      {u"cos"_s, u"co"_s},
        {u"fao"_s, u"fo"_s},        {u"fil"_s, u"tl"_s},      {u"fry"_s, u"fy"_s},     {u"hat"_s, u"ht"_s},      {u"hye"_s, u"hy"_s},
        {u"iku"_s, u"iu"_s},        {u"jav"_s, u"jv"_s},      {u"kmr"_s, u"ku"_s},     {u"kur"_s, u"ku"_s},      {u"lat"_s, u"la"_s},
        {u"ltz"_s, u"lb"_s},        {u"mkd"_s, u"mk"_s},      {u"mlt"_s, u"mt"_s},     {u"mon"_s, u"mn"_s},      {u"mri"_s, u"mi"_s},
        {u"msa"_s, u"ms"_s},        {u"oci"_s, u"oc"_s},      {u"pus"_s, u"ps"_s},     {u"que"_s, u"qu"_s},      {u"san"_s, u"sa"_s},
        {u"snd"_s, u"sd"_s},        {u"sqi"_s, u"sq"_s},      {u"sun"_s, u"su"_s},     {u"swa"_s, u"sw"_s},      {u"tat"_s, u"tt"_s},
        {u"tgk"_s, u"tg"_s},        {u"tgl"_s, u"tl"_s},      {u"tir"_s, u"ti"_s},     {u"ton"_s, u"to"_s},      {u"uig"_s, u"ug"_s},
        {u"uzb"_s, u"uz"_s},        {u"uzb_cyrl"_s, u"uz"_s}, {u"yid"_s, u"yi"_s},     {u"yor"_s, u"yo"_s},
    };

    if (tesseractCode == u"equ"_s) {
        return i18n("Math/Equation Detection");
    }
    if (tesseractCode == u"osd"_s) {
        return i18n("Orientation and Script Detection");
    }

    const QString isoCode = tesseractToIsoMap.value(tesseractCode);
    if (!isoCode.isEmpty()) {
        QLocale locale(isoCode);
        QString name = locale.nativeLanguageName();

        if (!name.isEmpty()) {
            name[0] = name[0].toUpper();
            return name;
        }

        QString languageName = QLocale::languageToString(locale.language());
        if (!languageName.isEmpty()) {
            languageName[0] = languageName[0].toUpper();
            return languageName;
        }
    }

    return tesseractCode;
}

OcrWorker::OcrWorker(QObject *parent)
    : QObject(parent)
{
}

void OcrWorker::processImage(const QImage &image, TessBaseAPI *tesseract, const TesseractRuntimeApi *runtimeApi)
{
    QMutexLocker locker(&m_mutex);

    if (!tesseract || !runtimeApi || image.isNull()) {
        Q_EMIT imageProcessed(QString(), false);
        return;
    }

    try {
        QImage rgbImage = image.convertToFormat(QImage::Format_RGB888);

        runtimeApi->setImage(tesseract, rgbImage.bits(), rgbImage.width(), rgbImage.height(), 3, rgbImage.bytesPerLine());

        if (runtimeApi->recognize(tesseract, nullptr) != 0) {
            Q_EMIT imageProcessed(QString(), false);
            return;
        }

        QStringList lines;
        TessResultIterator *iterator = runtimeApi->iterator(tesseract);

        if (iterator) {
            do {
                char *lineText = runtimeApi->iteratorText(iterator, RIL_TEXTLINE);
                if (lineText != nullptr) {
                    QString line = QString::fromUtf8(lineText).trimmed();
                    if (!line.isEmpty()) {
                        lines.append(line);
                    }
                    runtimeApi->deleteText(lineText);
                }
            } while (runtimeApi->iteratorNext(iterator, RIL_TEXTLINE) != 0);
            runtimeApi->iteratorDelete(iterator);
        }

        const QString result = lines.join(QLatin1Char('\n')).trimmed();
        Q_EMIT imageProcessed(result, true);
    } catch (const std::exception &e) {
        qCWarning(SPECTACLE_LOG) << "Exception in OCR worker:" << e.what();
        Q_EMIT imageProcessed(QString(), false);
    }
}
