/*
 *  SPDX-FileCopyrightText: 2025 Jhair Paris <dev@jhairparis.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "Config.h"
#include "TesseractCompatibility.h"

#include <QLibrary>
#include <QMutex>
#include <QStringList>

struct TesseractRuntimeApi {
    using CreateFunc = TessBaseAPI *(*)();
    using DeleteFunc = void (*)(TessBaseAPI *);
    using Init3Func = int (*)(TessBaseAPI *, const char *, const char *);
    using EndFunc = void (*)(TessBaseAPI *);
    using SetPageSegModeFunc = void (*)(TessBaseAPI *, TessPageSegMode);
    using GetDatapathFunc = const char *(*)(TessBaseAPI *);
    using SetImageFunc = void (*)(TessBaseAPI *, const unsigned char *, int, int, int, int);
    using RecognizeFunc = int (*)(TessBaseAPI *, ETEXT_DESC *);
    using GetIteratorFunc = TessResultIterator *(*)(TessBaseAPI *);
    using ResultGetUTF8TextFunc = char *(*)(const TessResultIterator *, TessPageIteratorLevel);
    using ResultNextFunc = int (*)(TessResultIterator *, TessPageIteratorLevel);
    using ResultDeleteFunc = void (*)(TessResultIterator *);
    using DeleteTextFunc = void (*)(char *);
    using DeleteTextArrayFunc = void (*)(char **);
    using VersionFunc = const char *(*)();
    using GetAvailableLanguagesAsVectorFunc = char **(*)(const TessBaseAPI *);

    CreateFunc create = nullptr;
    DeleteFunc dispose = nullptr;
    Init3Func init3 = nullptr;
    EndFunc end = nullptr;
    SetPageSegModeFunc setPageSegMode = nullptr;
    GetDatapathFunc datapath = nullptr;
    SetImageFunc setImage = nullptr;
    RecognizeFunc recognize = nullptr;
    GetIteratorFunc iterator = nullptr;
    ResultGetUTF8TextFunc iteratorText = nullptr;
    ResultNextFunc iteratorNext = nullptr;
    ResultDeleteFunc iteratorDelete = nullptr;
    DeleteTextFunc deleteText = nullptr;
    DeleteTextArrayFunc deleteTextArray = nullptr;
    VersionFunc version = nullptr;
    GetAvailableLanguagesAsVectorFunc getAvailableLanguagesAsVector = nullptr;
};

class TesseractRuntimeLoader
{
public:
    static TesseractRuntimeLoader &instance();

    bool ensureLoaded();
    bool isLoaded() const;
    const TesseractRuntimeApi *api() const;

private:
    TesseractRuntimeLoader();
    ~TesseractRuntimeLoader();

    bool loadLocked();
    bool resolveSymbols();
    bool validateLoadedVersion();
    QStringList candidateLibraryNames() const;

    Q_DISABLE_COPY_MOVE(TesseractRuntimeLoader)

    mutable QMutex m_mutex;
    bool m_loaded = false;
    QLibrary m_library;
    TesseractRuntimeApi m_api;
};
