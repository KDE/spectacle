/*
 *  SPDX-FileCopyrightText: 2025 Jhair Paris <dev@jhairparis.com>
 *  SPDX-FileCopyrightText: 2026 Kai Uwe Broulik <kde@broulik.de>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "OcrManager.h"
#include "settings.h"
#include "spectacle_debug.h"

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QMetaObject>
#include <QStandardPaths>
#include <QStringList>
#include <QThread>

#include <KLocalizedString>

#include <algorithm>

using namespace Qt::StringLiterals;

OcrManager *OcrManager::s_instance = nullptr;

OcrManager::OcrManager(QObject *parent)
    : QObject(parent)
    , m_workerThread(std::make_unique<QThread>())
    , m_timeoutTimer(new QTimer(this))
{
    m_workerThread->setObjectName(u"OcrThread"_s);

    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setInterval(30000);

    connect(m_timeoutTimer, &QTimer::timeout, this, [this]() {
        qCWarning(SPECTACLE_LOG) << "OCR recognition timed out";
        setStatus(OcrStatus::Error);
        Q_EMIT textRecognized({}, {}, false);
    });

    m_worker = new OcrWorker();
    m_worker->moveToThread(m_workerThread.get());

    connect(m_worker, &OcrWorker::initFinished, this, &OcrManager::handleInitializationComplete);
    connect(m_worker, &OcrWorker::languagesChanged, this, &OcrManager::handleLanguagesChanged);
    connect(m_worker, &OcrWorker::imageProcessed, this, &OcrManager::handleRecognitionComplete);

    m_workerThread->start();

    connect(Settings::self(), &Settings::ocrLanguagesChanged, this, [this]() {
        if (m_configSyncSuspended) {
            return;
        }

        const QStringList languages = Settings::ocrLanguages();

        if (languages != m_configuredLanguages) {
            setLanguagesByCode(languages);
        }
    });

    QMetaObject::invokeMethod(m_worker, &OcrWorker::init, Qt::QueuedConnection, Settings::ocrLanguages());
}

OcrManager::~OcrManager()
{
    m_workerThread->quit();
    if (!m_workerThread->wait(3000)) {
        qCWarning(SPECTACLE_LOG) << "Worker thread did not stop in time, terminating";
        m_workerThread->terminate();
        m_workerThread->wait();
    }

    delete m_worker;
    m_worker = nullptr;
}

OcrManager *OcrManager::instance()
{
    if (!s_instance) {
        s_instance = new OcrManager(qApp);
    }
    return s_instance;
}

void OcrManager::handleInitializationComplete(bool success, const QStringList &availableLanguages, const QStringList &activeLanguages)
{
    m_available = success;

    for (const QString &language : availableLanguages) {
        if (language == u"osd"_s) {
            m_languageNames.insert(language, i18nc("@item:inlistbox", "Orientation and Script Detection"));
        } else {
            m_languageNames.insert(language, tesseractLangName(language));
        }
    }
    m_activeLanguages = activeLanguages;
    m_availableLanguages = availableLanguages;

    if (!success) {
        qCWarning(SPECTACLE_LOG) << "OCR worker initialization failed";
        setStatus(OcrStatus::Error);
        return;
    }

    m_configuredLanguages = m_activeLanguages;

    // Persist fallback languages if the saved configuration
    // could not be used during initialization.
    if (Settings::ocrLanguages() != m_configuredLanguages) {
        Settings::setOcrLanguages(m_configuredLanguages);
        Settings::self()->save();
    }

    setStatus(OcrStatus::Ready);

    qCDebug(SPECTACLE_LOG) << "OCR worker initialized with:" << m_activeLanguages;
}

bool OcrManager::isAvailable() const
{
    return m_available;
}

OcrManager::OcrStatus OcrManager::status() const
{
    return m_status;
}

QMap<QString, QString> OcrManager::availableLanguagesWithNames() const
{
    return m_languageNames;
}

QString OcrManager::currentLanguageCode() const
{
    return m_activeLanguages.join(u"+"_s);
}

void OcrManager::setLanguagesByCode(const QStringList &languageCodes)
{
    if (languageCodes.isEmpty()) {
        qCWarning(SPECTACLE_LOG) << "No OCR languages specified";
        return;
    }

    if (!isAvailable()) {
        qCWarning(SPECTACLE_LOG) << "Cannot change OCR languages: engine unavailable";
        return;
    }

    m_pendingConfiguredLanguages = languageCodes;
    QMetaObject::invokeMethod(m_worker, &OcrWorker::setLanguages, Qt::QueuedConnection, languageCodes);
}

void OcrManager::handleLanguagesChanged(const QStringList &activeLanguages, bool success)
{
    if (!success) {
        qCWarning(SPECTACLE_LOG) << "Failed to change OCR languages";

        m_pendingConfiguredLanguages.clear();
        return;
    }

    m_activeLanguages = activeLanguages;

    if (!m_pendingConfiguredLanguages.isEmpty()) {
        m_configuredLanguages = activeLanguages;
        m_pendingConfiguredLanguages.clear();

        Settings::setOcrLanguages(m_configuredLanguages);
        Settings::self()->save();

        qCDebug(SPECTACLE_LOG) << "OCR languages changed to:" << currentLanguageCode();
    }
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

    m_shouldRestoreToConfigured = false;

    beginRecognition(image, m_configuredLanguages);
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

    // Store that we need to restore after recognition
    m_shouldRestoreToConfigured = (m_activeLanguages != m_configuredLanguages);

    beginRecognition(image, tempLanguages);
}

void OcrManager::beginRecognition(const QImage &image, const QStringList &languages)
{
    setStatus(OcrStatus::Processing);
    m_timeoutTimer->start();

    QMetaObject::invokeMethod(m_worker, &OcrWorker::processImage, Qt::QueuedConnection, image, languages);
}

void OcrManager::handleRecognitionComplete(const QString &text, const QStringList &activeLanguages, bool success)
{
    m_timeoutTimer->stop();

    m_activeLanguages = activeLanguages;

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
        const QStringList configuredLanguages = m_configuredLanguages;

        QMetaObject::invokeMethod(
            m_worker,
            [worker = m_worker, configuredLanguages]() {
                worker->setLanguages(configuredLanguages);
            },
            Qt::QueuedConnection);

        m_shouldRestoreToConfigured = false;
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

OcrWorker::~OcrWorker()
{
    if (m_tesseract) {
        m_tesseract->End();
    }
}

void OcrWorker::init(const QStringList &configuredLanguages)
{
    auto cleanup = qScopeGuard([this] {
        if (m_tesseract) {
            m_tesseract->End();
            m_tesseract.reset();
        }
        Q_EMIT initFinished(false, {}, {});
    });

    try {
        m_tesseract = std::make_unique<tesseract::TessBaseAPI>();

        if (m_tesseract->Init(nullptr, nullptr) != 0) {
            qCWarning(SPECTACLE_LOG) << "Failed to initialize Tesseract";
            m_tesseract.reset();
        } else if (setupAvailableLanguages()) {
            QStringList initialLanguages;

            for (const QString &language : configuredLanguages) {
                if (language != u"osd"_s && m_availableLanguages.contains(language) && !initialLanguages.contains(language)) {
                    initialLanguages.append(language);
                }
            }

            if (initialLanguages.isEmpty()) {
                initialLanguages = fallbackLanguages();
            }

            if (initialLanguages.isEmpty()) {
                qCWarning(SPECTACLE_LOG) << "No initial languages";
                return;
            }

            if (!applyLanguages(initialLanguages)) {
                qCWarning(SPECTACLE_LOG) << "Failed to initialize Tesseract with available languages";
                return;
            }
        }

        cleanup.dismiss();
        Q_EMIT initFinished(true, m_availableLanguages, m_activeLanguages);
    } catch (const std::exception &e) {
        qCWarning(SPECTACLE_LOG) << "Exception during Tesseract initialization:" << e.what();
    }
}

bool OcrWorker::setupAvailableLanguages()
{
    m_availableLanguages.clear();

    if (!m_tesseract) {
        return false;
    }

    std::vector<std::string> languages;
    m_tesseract->GetAvailableLanguagesAsVector(&languages);

    for (const auto &language : languages) {
        const QString code = QString::fromUtf8(language);

        if (!code.isEmpty() && !m_availableLanguages.contains(code)) {
            m_availableLanguages.append(code);
        }
    }

    std::sort(m_availableLanguages.begin(), m_availableLanguages.end());

    qCDebug(SPECTACLE_LOG) << "Detected OCR languages:" << m_availableLanguages;

    return !m_availableLanguages.isEmpty();
}

QStringList OcrWorker::fallbackLanguages() const
{
    const auto it = std::find_if(m_availableLanguages.cbegin(), m_availableLanguages.cend(), [](const QString &language) {
        return language != u"osd"_s;
    });

    if (it == m_availableLanguages.cend()) {
        return {};
    }

    return {*it};
}

bool OcrWorker::applyLanguages(const QStringList &languages)
{
    if (!m_tesseract || languages.isEmpty()) {
        return false;
    }

    QStringList validLanguages;

    for (const QString &language : languages) {
        if (language == u"osd"_s) {
            continue;
        }

        if (!m_availableLanguages.contains(language)) {
            qCWarning(SPECTACLE_LOG) << "OCR language not available:" << language;
            continue;
        }

        if (!validLanguages.contains(language)) {
            validLanguages.append(language);
        }
    }

    if (validLanguages.size() > OcrManager::MAX_OCR_LANGUAGES) {
        validLanguages = validLanguages.mid(0, OcrManager::MAX_OCR_LANGUAGES);
    }

    if (validLanguages.isEmpty()) {
        return false;
    }

    if (validLanguages == m_activeLanguages) {
        return true;
    }

    // Verify all requested traineddata files exist.
    const char *datapath = m_tesseract->GetDatapath();
    const QString tessdataPath = datapath ? QString::fromUtf8(datapath) : QString();

    if (tessdataPath.isEmpty()) {
        qCWarning(SPECTACLE_LOG) << "Tessdata path is empty";
        return false;
    }

    QDir tessdataDir{tessdataPath};
    for (const QString &language : std::as_const(validLanguages)) {
        const QString path = tessdataDir.filePath(language + u".traineddata"_s);

        if (!QFile::exists(path)) {
            qCWarning(SPECTACLE_LOG) << "Language file not found:" << path;
            return false;
        }
    }

    m_tesseract->End();

    const QByteArray languageData = validLanguages.join(u"+"_s).toUtf8();

    if (m_tesseract->Init(nullptr, languageData.constData()) != 0) {
        qCWarning(SPECTACLE_LOG) << "Failed to initialize Tesseract with:" << validLanguages;

        m_activeLanguages.clear();
        return false;
    }

    m_tesseract->SetPageSegMode(tesseract::PSM_AUTO);
    m_activeLanguages = validLanguages;

    qCDebug(SPECTACLE_LOG) << "Applied OCR languages:" << m_activeLanguages;

    return true;
}

void OcrWorker::setLanguages(const QStringList &languages)
{
    const bool success = applyLanguages(languages);

    Q_EMIT languagesChanged(m_activeLanguages, success);
}

void OcrWorker::processImage(const QImage &image, const QStringList &languages)
{
    if (!m_tesseract || image.isNull()) {
        Q_EMIT imageProcessed({}, m_activeLanguages, false);
        return;
    }

    try {
        if (!applyLanguages(languages)) {
            Q_EMIT imageProcessed({}, m_activeLanguages, false);
            return;
        }

        QImage rgbImage = image.convertToFormat(QImage::Format_RGB888);

        m_tesseract->SetImage(rgbImage.bits(), rgbImage.width(), rgbImage.height(), 3, rgbImage.bytesPerLine());

        if (m_tesseract->Recognize(nullptr) != 0) {
            Q_EMIT imageProcessed({}, m_activeLanguages, false);
            return;
        }

        QStringList lines;
        TessResultIterator *iterator = m_tesseract->GetIterator();

        if (iterator) {
            do {
                char *lineText = iterator->GetUTF8Text(tesseract::RIL_TEXTLINE);
                if (lineText != nullptr) {
                    QString line = QString::fromUtf8(lineText).trimmed();
                    if (!line.isEmpty()) {
                        lines.append(line);
                    }
                    delete [] lineText;
                }
            } while (iterator->Next(tesseract::RIL_TEXTLINE) != 0);
            delete iterator;
        }

        const QString result = lines.join(QLatin1Char('\n')).trimmed();
        Q_EMIT imageProcessed(result, m_activeLanguages, true);
    } catch (const std::exception &e) {
        qCWarning(SPECTACLE_LOG) << "Exception in OCR worker:" << e.what();
        Q_EMIT imageProcessed({}, m_activeLanguages, false);
    }
}

#include "moc_OcrManager.cpp"
