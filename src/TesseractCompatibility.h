/*
 *  SPDX-FileCopyrightText: 2025 Jhair Paris <dev@jhairparis.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#if __has_include(<tesseract/capi.h>)
#include <tesseract/capi.h>

using TessPageSegMode = tesseract::PageSegMode;
using TessPageIteratorLevel = tesseract::PageIteratorLevel;

static constexpr auto PSM_AUTO = tesseract::PSM_AUTO;
static constexpr auto RIL_TEXTLINE = tesseract::RIL_TEXTLINE;

#else
extern "C" {
struct TessBaseAPI;
struct TessResultIterator;
struct ETEXT_DESC;
}

enum TessPageSegMode {
    PSM_OSD_ONLY,
    PSM_AUTO_OSD,
    PSM_AUTO_ONLY,
    PSM_AUTO,
    PSM_SINGLE_COLUMN,
    PSM_SINGLE_BLOCK_VERT_TEXT,
    PSM_SINGLE_BLOCK,
    PSM_SINGLE_LINE,
    PSM_SINGLE_WORD,
    PSM_CIRCLE_WORD,
    PSM_SINGLE_CHAR,
    PSM_SPARSE_TEXT,
    PSM_SPARSE_TEXT_OSD,
    PSM_RAW_LINE,
    PSM_COUNT
};

enum TessPageIteratorLevel {
    RIL_BLOCK,
    RIL_PARA,
    RIL_TEXTLINE,
    RIL_WORD,
    RIL_SYMBOL
};
#endif
