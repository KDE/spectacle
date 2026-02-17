/*
 *  SPDX-FileCopyrightText: 2025 Jhair Paris <dev@jhairparis.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "TesseractRuntimeLoader.h"

#include "spectacle_debug.h"

#include <QMutexLocker>
#include <type_traits>

TesseractRuntimeLoader &TesseractRuntimeLoader::instance()
{
    static TesseractRuntimeLoader s_instance;
    return s_instance;
}

TesseractRuntimeLoader::TesseractRuntimeLoader() = default;
TesseractRuntimeLoader::~TesseractRuntimeLoader()
{
    if (m_library.isLoaded()) {
        m_library.unload();
    }
}

bool TesseractRuntimeLoader::ensureLoaded()
{
    QMutexLocker locker(&m_mutex);
    if (m_loaded) {
        return true;
    }
    return loadLocked();
}

bool TesseractRuntimeLoader::isLoaded() const
{
    QMutexLocker locker(&m_mutex);
    return m_loaded;
}

const TesseractRuntimeApi *TesseractRuntimeLoader::api() const
{
    QMutexLocker locker(&m_mutex);
    return m_loaded ? &m_api : nullptr;
}

bool TesseractRuntimeLoader::loadLocked()
{
    const auto candidates = candidateLibraryNames();
    for (const QString &candidate : candidates) {
        // From https://doc.qt.io/qt-6/qlibrary.html :
        // QLibrary tries the name with different platform-specific file prefixes,
        // like "lib" on Unix and Mac, and suffixes, like ".so" on Unix,
        // ".dylib" on the Mac, or ".dll" on Windows.
        m_library.setFileName(candidate);
        m_library.setLoadHints(QLibrary::ExportExternalSymbolsHint | QLibrary::PreventUnloadHint);

        if (!m_library.load()) {
            qCWarning(SPECTACLE_LOG) << "Unable to load Tesseract candidate" << candidate << ':' << m_library.errorString();
            continue;
        }

        qCInfo(SPECTACLE_LOG) << "Attempting to use Tesseract library" << candidate;

        if (!resolveSymbols()) {
            m_library.unload();
            continue;
        }

        if (!validateLoadedVersion()) {
            m_library.unload();
            continue;
        }

        m_loaded = true;
        qCInfo(SPECTACLE_LOG) << "Loaded Tesseract runtime library from" << m_library.fileName();
        return true;
    }

    qCWarning(SPECTACLE_LOG) << "Unable to locate a suitable Tesseract shared library";
    return false;
}

bool TesseractRuntimeLoader::resolveSymbols()
{
    auto resolve = [this](auto &target, const char *symbol) {
        target = reinterpret_cast<std::remove_reference_t<decltype(target)>>(m_library.resolve(symbol));
        if (!target) {
            qCWarning(SPECTACLE_LOG) << "Failed to resolve" << symbol << "from" << m_library.fileName();
            return false;
        }
        return true;
    };

    if (!resolve(m_api.create, "TessBaseAPICreate")) {
        return false;
    }
    if (!resolve(m_api.dispose, "TessBaseAPIDelete")) {
        return false;
    }
    if (!resolve(m_api.init3, "TessBaseAPIInit3")) {
        return false;
    }
    if (!resolve(m_api.end, "TessBaseAPIEnd")) {
        return false;
    }
    if (!resolve(m_api.setPageSegMode, "TessBaseAPISetPageSegMode")) {
        return false;
    }
    if (!resolve(m_api.datapath, "TessBaseAPIGetDatapath")) {
        return false;
    }
    if (!resolve(m_api.setImage, "TessBaseAPISetImage")) {
        return false;
    }
    if (!resolve(m_api.recognize, "TessBaseAPIRecognize")) {
        return false;
    }
    if (!resolve(m_api.iterator, "TessBaseAPIGetIterator")) {
        return false;
    }
    if (!resolve(m_api.iteratorText, "TessResultIteratorGetUTF8Text")) {
        return false;
    }
    if (!resolve(m_api.iteratorNext, "TessResultIteratorNext")) {
        return false;
    }
    if (!resolve(m_api.iteratorDelete, "TessResultIteratorDelete")) {
        return false;
    }
    if (!resolve(m_api.deleteText, "TessDeleteText")) {
        return false;
    }
    if (!resolve(m_api.version, "TessVersion")) {
        return false;
    }
    if (!resolve(m_api.getAvailableLanguagesAsVector, "TessBaseAPIGetAvailableLanguagesAsVector")) {
        return false;
    }
    if (!resolve(m_api.deleteTextArray, "TessDeleteTextArray")) {
        return false;
    }

    return true;
}

bool TesseractRuntimeLoader::validateLoadedVersion()
{
    constexpr int kMinSupportedMajor = 4;
    constexpr int kMaxSupportedMajor = 5;

    if (!m_api.version) {
        qCWarning(SPECTACLE_LOG) << "Tesseract runtime missing TessVersion symbol";
        return false;
    }

    const char *versionPtr = m_api.version();
    const QString versionString = versionPtr ? QString::fromLatin1(versionPtr) : QString();

    if (versionString.isEmpty()) {
        qCWarning(SPECTACLE_LOG) << "Unable to determine Tesseract runtime version";
        return false;
    }

    const QString majorComponent = versionString.section(QLatin1Char('.'), 0, 0);
    bool ok = false;
    const int majorVersion = majorComponent.toInt(&ok);

    if (!ok) {
        qCWarning(SPECTACLE_LOG) << "Failed to parse Tesseract version" << versionString;
        return false;
    }

    if (majorVersion < kMinSupportedMajor || majorVersion > kMaxSupportedMajor) {
        qCWarning(SPECTACLE_LOG) << "Unsupported Tesseract version" << versionString << "(supported major versions" << kMinSupportedMajor << "-"
                                 << kMaxSupportedMajor << ')';
        return false;
    }

    qCInfo(SPECTACLE_LOG) << "Detected Tesseract version" << versionString;
    return true;
}

QStringList TesseractRuntimeLoader::candidateLibraryNames() const
{
    return {QStringLiteral("tesseract")};
}
