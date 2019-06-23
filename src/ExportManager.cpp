/* This file is part of Spectacle, the KDE screenshot utility
 * Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ExportManager.h"

#include <QDir>
#include <QMimeData>
#include <QMimeDatabase>
#include <QImageWriter>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QApplication>
#include <QClipboard>
#include <QPainter>
#include <QFileDialog>
#include <QBuffer>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QMimeData>
#include <QString>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KIO/ListJob>
#include <KIO/MkpathJob>
#include <KIO/FileCopyJob>
#include <KIO/StatJob>

#include "SpectacleConfig.h"

ExportManager::ExportManager(QObject *parent) :
    QObject(parent),
    mSavePixmap(QPixmap()),
    mTempFile(QUrl()),
    mTempDir(nullptr)
{
    connect(this, &ExportManager::imageSaved, [](const QUrl &savedAt) {
        SpectacleConfig::instance()->setLastSaveFile(savedAt);
    });
}

ExportManager::~ExportManager()
{
    delete mTempDir;
}

ExportManager* ExportManager::instance()
{
    static ExportManager instance;
    return &instance;
}

// screenshot pixmap setter and getter

QPixmap ExportManager::pixmap() const
{
    return mSavePixmap;
}

void ExportManager::setWindowTitle(const QString &windowTitle)
{
    mWindowTitle = windowTitle;
}

QString ExportManager::windowTitle() const
{
    return mWindowTitle;
}

Spectacle::CaptureMode ExportManager::captureMode() const
{
    return mCaptureMode;
}

void ExportManager::setCaptureMode(const Spectacle::CaptureMode &theCaptureMode)
{
    mCaptureMode = theCaptureMode;
}

void ExportManager::setPixmap(const QPixmap &pixmap)
{
    mSavePixmap = pixmap;

    // reset our saved tempfile
    if (mTempFile.isValid()) {
        mUsedTempFileNames.append(mTempFile);
        QFile file(mTempFile.toLocalFile());
        file.remove();
        mTempFile = QUrl();
    }
}

void ExportManager::updatePixmapTimestamp()
{
	mPixmapTimestamp = QDateTime::currentDateTime();
}

void ExportManager::setTimestamp(const QDateTime &timestamp)
{
    mPixmapTimestamp = timestamp;
}

// native file save helpers

QString ExportManager::defaultSaveLocation() const
{
    QString savePath = SpectacleConfig::instance()->defaultSaveLocation().toString();
    savePath = QDir::cleanPath(savePath);

    QDir savePathDir(savePath);
    if (!(savePathDir.exists())) {
        savePathDir.mkpath(QStringLiteral("."));
    }

    return savePath;
}

QUrl ExportManager::getAutosaveFilename()
{
    const QString baseDir = defaultSaveLocation();
    const QDir baseDirPath(baseDir);
    const QString filename = makeAutosaveFilename();
    const QString fullpath = autoIncrementFilename(baseDirPath.filePath(filename),
                                                   SpectacleConfig::instance()->saveImageFormat(),
                                                   &ExportManager::isFileExists);

    const QUrl fileNameUrl = QUrl::fromUserInput(fullpath);
    if (fileNameUrl.isValid()) {
        return fileNameUrl;
    } else {
        return QUrl();
    }
}

QString ExportManager::truncatedFilename(QString const &filename)
{
    QString result = filename;
    constexpr auto maxFilenameLength = 255;
    constexpr auto maxExtensionLength = 5;  // For example, ".jpeg"
    constexpr auto maxCounterLength = 20;  // std::numeric_limits<quint64>::max() == 18446744073709551615
    constexpr auto maxLength = maxFilenameLength - maxCounterLength - maxExtensionLength;
    result.truncate(maxLength);
    return result;
}

QString ExportManager::makeAutosaveFilename()
{
    return formatFilename(SpectacleConfig::instance()->autoSaveFilenameFormat());
}

QString ExportManager::formatFilename(const QString &nameTemplate)
{
    const QDateTime timestamp = mPixmapTimestamp;
    QString baseName = nameTemplate;
    const QString baseDir = defaultSaveLocation();
    QString title;

    if (mCaptureMode == Spectacle::CaptureMode::ActiveWindow ||
        mCaptureMode == Spectacle::CaptureMode::TransientWithParent ||
        mCaptureMode == Spectacle::CaptureMode::WindowUnderCursor) {
        title = mWindowTitle.replace(QLatin1String("/"), QLatin1String("_"));  // POSIX doesn't allow "/" in filenames
    } else {
        // Remove '%T' with separators around it
        const auto wordSymbol = QStringLiteral(R"(\p{L}\p{M}\p{N})");
        const auto separator = QStringLiteral("([^%1]+)").arg(wordSymbol);
        const auto re = QRegularExpression(QStringLiteral("(.*?)(%1%T|%T%1)(.*?)").arg(separator));
        baseName.replace(re, QStringLiteral(R"(\1\5)"));
    }

    QString result = baseName.replace(QLatin1String("%Y"), timestamp.toString(QStringLiteral("yyyy")))
                             .replace(QLatin1String("%y"), timestamp.toString(QStringLiteral("yy")))
                             .replace(QLatin1String("%M"), timestamp.toString(QStringLiteral("MM")))
                             .replace(QLatin1String("%D"), timestamp.toString(QStringLiteral("dd")))
                             .replace(QLatin1String("%H"), timestamp.toString(QStringLiteral("hh")))
                             .replace(QLatin1String("%m"), timestamp.toString(QStringLiteral("mm")))
                             .replace(QLatin1String("%S"), timestamp.toString(QStringLiteral("ss")))
                             .replace(QLatin1String("%T"), title);
                             
    // check if basename includes %[N]d token for sequential file numbering
    QRegularExpression paddingRE;
    paddingRE.setPattern(QStringLiteral("%(\\d*)d"));
    QRegularExpressionMatch paddingMatch;
                             
    while (result.indexOf(paddingRE, 0, &paddingMatch) > -1) {
        int highestFileNumber = 0;

        // determine padding value
        int paddedLength = 1;
        if (!paddingMatch.captured(1).isEmpty()) {
            paddedLength = paddingMatch.captured(1).toInt();
        }

        // search save directory for files
        QDir dir(baseDir);
        const QStringList fileNames = dir.entryList(QDir::Files, QDir::Name);
        
        // if there are files in the directory...
        if (fileNames.length() > 0) { 
            QString resultCopy = result;
            QRegularExpression fileNumberRE;
            const QString replacement = QStringLiteral("(\\d{").append(QString::number(paddedLength)).append(QLatin1String(",})"));
            const QString fullNameMatch = QStringLiteral("^").append(resultCopy.replace(paddingMatch.captured(),replacement)).append(QStringLiteral("\\..*$"));
            fileNumberRE.setPattern(fullNameMatch);

            // ... check the file names for string matching token with padding specified in result
            const QStringList filteredFiles = fileNames.filter(fileNumberRE);

            // if there are files in the direcory that look like the file name with sequential numbering
            if (filteredFiles.length() > 0) {
                // loop through filtered file names looking for highest number
                for (const QString &filteredFile: filteredFiles) {
                    int currentFileNumber = fileNumberRE.match(filteredFile).captured(1).toInt();
                    if (currentFileNumber > highestFileNumber) {
                        highestFileNumber = currentFileNumber;
                    }
                }
            }
        }
        // replace placeholder with next number padded
        const QString nextFileNumberPadded = QString::number(highestFileNumber + 1).rightJustified(paddedLength, QLatin1Char('0'));
        result = result.replace(paddingMatch.captured(), nextFileNumberPadded);
    }

    // Remove leading and trailing '/'
    while (result.startsWith(QLatin1Char('/'))) {
        result.remove(0, 1);
    }
    while (result.endsWith(QLatin1Char('/'))) {
        result.chop(1);
    }

    if (result.isEmpty()) {
        result = SpectacleConfig::instance()->defaultFilename();
    }
    return truncatedFilename(result);
}

QString ExportManager::autoIncrementFilename(const QString &baseName, const QString &extension,
                                             FileNameAlreadyUsedCheck isFileNameUsed)
{
    QString result = truncatedFilename(baseName) + QLatin1Literal(".") + extension;
    if (!((this->*isFileNameUsed)(QUrl::fromUserInput(result)))) {
        return result;
    }

    QString fileNameFmt = truncatedFilename(baseName) + QStringLiteral("-%1.");
    for (quint64 i = 1; i < std::numeric_limits<quint64>::max(); i++) {
        result = fileNameFmt.arg(i) + extension;
        if (!((this->*isFileNameUsed)(QUrl::fromUserInput(result)))) {
            return result;
        }
    }

    // unlikely this will ever happen, but just in case we've run
    // out of numbers

    result = fileNameFmt.arg(QStringLiteral("OVERFLOW-") + QString::number(qrand() % 10000));
    return truncatedFilename(result) + extension;
}

QString ExportManager::makeSaveMimetype(const QUrl &url)
{
    QMimeDatabase mimedb;
    QString type = mimedb.mimeTypeForUrl(url).preferredSuffix();

    if (type.isEmpty()) {
        return SpectacleConfig::instance()->saveImageFormat();
    }
    return type;
}

bool ExportManager::writeImage(QIODevice *device, const QByteArray &format)
{
    QImageWriter imageWriter(device, format);
    imageWriter.setQuality(SpectacleConfig::instance()->compressionQuality());
    /** Set compression 50 if the format is png. Otherwise if no compression value is specified
     *  it will fallback to using quality (QTBUG-43618) and produce huge files.
     *  See also qpnghandler.cpp#n1075. The other formats that do compression seem to have it
     *  enabled by default and only disabled if compression is set to 0, also any value except 0
     *  has the same effect for them.
     */
    if (format == "png") {
        imageWriter.setCompression(50);
    }
    if (!(imageWriter.canWrite())) {
        emit errorMessage(i18n("QImageWriter cannot write image: %1", imageWriter.errorString()));
        return false;
    }

    return imageWriter.write(mSavePixmap.toImage());
}

bool ExportManager::localSave(const QUrl &url, const QString &mimetype)
{
    // Create save directory if it doesn't exist
    const QUrl dirPath(url.adjusted(QUrl::RemoveFilename));
    const QDir dir(dirPath.path());

    if (!dir.mkpath(QStringLiteral("."))) {
        emit errorMessage(xi18nc("@info",
                                 "Cannot save screenshot because creating "
                                 "the directory failed:<nl/><filename>%1</filename>",
                                 dirPath.path()));
        return false;
    }

    QFile outputFile(url.toLocalFile());

    outputFile.open(QFile::WriteOnly);
    if(!writeImage(&outputFile, mimetype.toLatin1())) {
        emit errorMessage(i18n("Cannot save screenshot. Error while writing file."));
        return false;
    }
    return true;
}

bool ExportManager::remoteSave(const QUrl &url, const QString &mimetype)
{

    // Check if remote save directory exists
    const QUrl dirPath(url.adjusted(QUrl::RemoveFilename));
    KIO::ListJob *listJob = KIO::listDir(dirPath);
    listJob->exec();

    if (listJob->error() != KJob::NoError) {
        // Create remote save directory
        KIO::MkpathJob *mkpathJob = KIO::mkpath(dirPath, QUrl(defaultSaveLocation()));
        mkpathJob->exec();

        if (mkpathJob->error() != KJob::NoError) {
            emit errorMessage(xi18nc("@info",
                                     "Cannot save screenshot because creating the "
                                     "remote directory failed:<nl/><filename>%1</filename>",
                                     dirPath.path()));
            return false;
        }
    }

    QTemporaryFile tmpFile;

    if (tmpFile.open()) {
        if(!writeImage(&tmpFile, mimetype.toLatin1())) {
            emit errorMessage(i18n("Cannot save screenshot. Error while writing temporary local file."));
            return false;
        }

        KIO::FileCopyJob *uploadJob = KIO::file_copy(QUrl::fromLocalFile(tmpFile.fileName()), url);
        uploadJob->exec();

        if (uploadJob->error() != KJob::NoError) {
            emit errorMessage(i18n("Unable to save image. Could not upload file to remote location."));
            return false;
        }
        return true;
    }

    return false;
}

QUrl ExportManager::tempSave()
{
    // if we already have a temp file saved, use that
    if (mTempFile.isValid()) {
        if (QFile(mTempFile.toLocalFile()).exists()) {
            return mTempFile;
        }
    }

    if (!mTempDir) {
        mTempDir = new QTemporaryDir(QDir::tempPath() + QDir::separator() + QStringLiteral("Spectacle.XXXXXX"));
    }
    if (mTempDir && mTempDir->isValid()) {
        // create the temporary file itself with normal file name and also unique one for this session
        // supports the use-case of creating multiple screenshots in a row
        // and exporting them to the same destination e.g. via clipboard,
        // where the temp file name is used as filename suggestion
        const QString baseFileName = mTempDir->path() + QDir::separator() + makeAutosaveFilename();

        QString mimetype = makeSaveMimetype(QUrl(baseFileName));
        const QString fileName = autoIncrementFilename(baseFileName, mimetype,
                                                       &ExportManager::isTempFileAlreadyUsed);
        QFile tmpFile(fileName);
        if (tmpFile.open(QFile::WriteOnly)) {
            if(writeImage(&tmpFile, mimetype.toLatin1())) {
                mTempFile = QUrl::fromLocalFile(tmpFile.fileName());
                // try to make sure 3rd-party which gets the url of the temporary file e.g. on export
                // properly treats this as readonly, also hide from other users
                tmpFile.setPermissions(QFile::ReadUser);
                return mTempFile;
            }
        }
    }

    emit errorMessage(i18n("Cannot save screenshot. Error while writing temporary local file."));
    return QUrl();
}

bool ExportManager::save(const QUrl &url)
{
    if (!(url.isValid())) {
        emit errorMessage(i18n("Cannot save screenshot. The save filename is invalid."));
        return false;
    }

    QString mimetype = makeSaveMimetype(url);
    if (url.isLocalFile()) {
        return localSave(url, mimetype);
    }
    return remoteSave(url, mimetype);
}

bool ExportManager::isFileExists(const QUrl &url) const
{
    if (!(url.isValid())) {
        return false;
    }

    KIO::StatJob * existsJob = KIO::stat(url, KIO::StatJob::DestinationSide, 0);
    existsJob->exec();

    return (existsJob->error() == KJob::NoError);
}

bool ExportManager::isTempFileAlreadyUsed(const QUrl &url) const
{
    return mUsedTempFileNames.contains(url);
}

// save slots

void ExportManager::doSave(const QUrl &url, bool notify)
{
    if (mSavePixmap.isNull()) {
        emit errorMessage(i18n("Cannot save an empty screenshot image."));
        return;
    }

    QUrl savePath = url.isValid() ? url : getAutosaveFilename();
    if (save(savePath)) {
        QDir dir(savePath.path());
        dir.cdUp();
        SpectacleConfig::instance()->setLastSaveFile(savePath);

        emit imageSaved(savePath);
        if (notify) {
            emit forceNotify(savePath);
        }
    }
}

bool ExportManager::doSaveAs(QWidget *parentWindow, bool notify)
{
    QStringList supportedFilters;
    SpectacleConfig *config = SpectacleConfig::instance();

    // construct the supported mimetype list
    Q_FOREACH (auto mimeType, QImageWriter::supportedMimeTypes()) {
        supportedFilters.append(QString::fromUtf8(mimeType).trimmed());
    }

    // construct the file name
    const QString filenameExtension = SpectacleConfig::instance()->saveImageFormat();
    const QString mimetype = QMimeDatabase().mimeTypeForFile(QStringLiteral("~/fakefile.") + filenameExtension, QMimeDatabase::MatchExtension).name();
    QFileDialog dialog(parentWindow);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setDirectoryUrl(config->lastSaveAsLocation());
    dialog.selectFile(makeAutosaveFilename() + QStringLiteral(".") + filenameExtension);
    dialog.setDefaultSuffix(QStringLiteral(".") + filenameExtension);
    dialog.setMimeTypeFilters(supportedFilters);
    dialog.selectMimeTypeFilter(mimetype);

    // launch the dialog
    if (dialog.exec() == QFileDialog::Accepted) {
        const QUrl saveUrl = dialog.selectedUrls().constFirst();
        if (saveUrl.isValid()) {
            if (save(saveUrl)) {
                emit imageSaved(saveUrl);
                config->setLastSaveAsFile(saveUrl);

                if (notify) {
                    emit forceNotify(saveUrl);
                }
                return true;
            }
        }
    }
    return false;
}

// misc helpers

void ExportManager::doCopyToClipboard(bool notify)
{
    auto data = new QMimeData();
    data->setImageData(mSavePixmap.toImage());
    data->setData(QStringLiteral("x-kde-force-image-copy"), QByteArray());
    QApplication::clipboard()->setMimeData(data, QClipboard::Clipboard);

    if (notify) {
        emit forceNotify(QUrl());
    }
}

void ExportManager::doPrint(QPrinter *printer)
{
    QPainter painter;

    if (!(painter.begin(printer))) {
        emit errorMessage(i18n("Printing failed. The printer failed to initialize."));
        delete printer;
        return;
    }

    QRect devRect(0, 0, printer->width(), printer->height());
    QPixmap pixmap = mSavePixmap.scaled(devRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QRect srcRect = pixmap.rect();
    srcRect.moveCenter(devRect.center());

    painter.drawPixmap(srcRect.topLeft(), pixmap);
    painter.end();

    delete printer;
    return;
}

const QMap<QString, KLocalizedString> ExportManager::filenamePlaceholders {
    {QStringLiteral("%Y"), ki18nc(
            "A placeholder in the user configurable filename will replaced by the specified value",
            "Year (4 digit)")},
    {QStringLiteral("%y"), ki18nc(
            "A placeholder in the user configurable filename will replaced by the specified value",
            "Year (2 digit)")},
    {QStringLiteral("%M"), ki18nc(
            "A placeholder in the user configurable filename will replaced by the specified value",
            "Month")},
    {QStringLiteral("%D"), ki18nc(
            "A placeholder in the user configurable filename will replaced by the specified value",
            "Day")},
    {QStringLiteral("%H"), ki18nc(
            "A placeholder in the user configurable filename will replaced by the specified value",
            "Hour")},
    {QStringLiteral("%m"), ki18nc(
            "A placeholder in the user configurable filename will replaced by the specified value",
            "Minute")},
    {QStringLiteral("%S"), ki18nc(
            "A placeholder in the user configurable filename will replaced by the specified value",
            "Second")},
    {QStringLiteral("%T"), ki18nc(
            "A placeholder in the user configurable filename will replaced by the specified value",
            "Window Title")},
    {QStringLiteral("%d"), ki18nc(
            "A placeholder in the user configurable filename will replaced by the specified value",
            "Sequential numbering")},
    {QStringLiteral("%Nd"), ki18nc(
            "A placeholder in the user configurable filename will replaced by the specified value",
            "Sequential numbering, padded out to N digits")},
};
