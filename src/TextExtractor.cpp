// SPDX-FileCopyrightText: 2021 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "TextExtractor.h"

#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>

#include <KLocalizedString>
#include <KNotification>
#include <QApplication>
#include <QClipboard>
#include <QObject>

void TextExtractor::doExtract(const QString &location)
{
    tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
    if (api->Init(NULL, "eng")) {
        Q_EMIT errorOccured(i18n("There was a problem extracting the text"));
        return;
    }

    Pix *image = pixRead(location.toLocal8Bit().data());
    api->SetImage(image);
    auto result = QString::fromUtf8(api->GetUTF8Text());
    api->End();

    QApplication::clipboard()->setText(result);
    auto lNotify = new KNotification(QStringLiteral("imageExtractionFinished"));
    lNotify->setText(i18n("The text inside the image was succesfully copied to your clipboard"));
    lNotify->sendEvent();
}
