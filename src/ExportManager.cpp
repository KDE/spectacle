/*
 *  Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "ExportManager.h"

#include <QDir>
#include <QMimeDatabase>
#include <QImageWriter>
#include <QTemporaryFile>
#include <QApplication>
#include <QClipboard>
#include <QPainter>
#include <QFileDialog>
#include <QBuffer>

#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KIO/FileCopyJob>
#include <KIO/StatJob>

ExportManager::ExportManager(QObject *parent) :
    QObject(parent),
    mSavePixmap(QPixmap()),
    mTempFile(QUrl())
{}

ExportManager::~ExportManager()
{}

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

QString ExportManager::pixmapDataUri() const
{
    QImage image = mSavePixmap.toImage();
    QByteArray imageData;

    // write the image into the QByteArray using a QBuffer

    {
        QBuffer dataBuf(&imageData);
        dataBuf.open(QBuffer::WriteOnly);
        image.save(&dataBuf, "PNG");
    }

    // compose the data uri and return it

    QString uri = QStringLiteral("data:image/png;base64,") + QString::fromLatin1(imageData.toBase64());
    return uri;
}

void ExportManager::setPixmap(const QPixmap &pixmap)
{
    mSavePixmap = pixmap;

    // reset our saved tempfile
    if (mTempFile.isValid()) {
        QFile file(mTempFile.toLocalFile());
        file.remove();
        mTempFile = QUrl();
    }
}

// native file save helpers

QString ExportManager::saveLocation() const
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("spectaclerc"));
    KConfigGroup generalConfig = KConfigGroup(config, "General");

    QString savePath = generalConfig.readPathEntry(
                "default-save-location", QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    if (savePath.isEmpty() || savePath.isNull()) {
        savePath = QDir::homePath();
    }
    savePath = QDir::cleanPath(savePath);

    QDir savePathDir(savePath);
    if (!(savePathDir.exists())) {
        savePathDir.mkpath(QStringLiteral("."));
        generalConfig.writePathEntry("last-saved-to", savePath);
    }

    return savePath;
}

void ExportManager::setSaveLocation(const QString &savePath)
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("spectaclerc"));
    KConfigGroup generalConfig = KConfigGroup(config, "General");

    generalConfig.writePathEntry("last-saved-to", savePath);
}

QUrl ExportManager::getAutosaveFilename()
{
    const QString baseDir = saveLocation();
    const QDir baseDirPath(baseDir);
    const QString filename = makeAutosaveFilename();
    const QString fullpath = autoIncrementFilename(baseDirPath.filePath(filename), QStringLiteral("png"));

    const QUrl fileNameUrl = QUrl::fromUserInput(fullpath);
    if (fileNameUrl.isValid()) {
        return fileNameUrl;
    } else {
        return QUrl();
    }
}

QString ExportManager::makeAutosaveFilename()
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("spectaclerc"));
    KConfigGroup generalConfig = KConfigGroup(config, "General");

    const QDateTime timestamp = QDateTime::currentDateTime();
    QString baseName = generalConfig.readEntry("save-filename-format", "Screenshot_%Y%M%D_%H%m%S");

    return baseName.replace(QLatin1String("%Y"), timestamp.toString(QStringLiteral("yyyy")))
                   .replace(QLatin1String("%y"), timestamp.toString(QStringLiteral("yy")))
                   .replace(QLatin1String("%M"), timestamp.toString(QStringLiteral("MM")))
                   .replace(QLatin1String("%D"), timestamp.toString(QStringLiteral("dd")))
                   .replace(QLatin1String("%H"), timestamp.toString(QStringLiteral("hh")))
                   .replace(QLatin1String("%m"), timestamp.toString(QStringLiteral("mm")))
                   .replace(QLatin1String("%S"), timestamp.toString(QStringLiteral("ss")));
}

QString ExportManager::autoIncrementFilename(const QString &baseName, const QString &extension)
{
    if (!(isFileExists(QUrl::fromUserInput(baseName + '.' + extension)))) {
        return baseName + '.' + extension;
    }

    QString fileNameFmt(baseName + "-%1." + extension);
    for (quint64 i = 1; i < std::numeric_limits<quint64>::max(); i++) {
        if (!(isFileExists(QUrl::fromUserInput(fileNameFmt.arg(i))))) {
            return fileNameFmt.arg(i);
        }
    }

    // unlikely this will ever happen, but just in case we've run
    // out of numbers

    return fileNameFmt.arg("OVERFLOW-" + (qrand() % 10000));
}

QString ExportManager::makeSaveMimetype(const QUrl &url)
{
    QMimeDatabase mimedb;
    QString type = mimedb.mimeTypeForUrl(url).preferredSuffix();

    if (type.isEmpty()) {
        return QStringLiteral("png");
    }
    return type;
}

bool ExportManager::writeImage(QIODevice *device, const QByteArray &format)
{
    QImageWriter imageWriter(device, format);
    if (!(imageWriter.canWrite())) {
        emit errorMessage(i18n("QImageWriter cannot write image: ") + imageWriter.errorString());
        return false;
    }

    return imageWriter.write(mSavePixmap.toImage());
}

bool ExportManager::localSave(const QUrl &url, const QString &mimetype)
{
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

QUrl ExportManager::tempSave(const QString &mimetype)
{
    // if we already have a temp file saved, use that
    if (mTempFile.isValid()) {
        if (QFile(mTempFile.toLocalFile()).exists()) {
            return mTempFile;
        }
    }

    QTemporaryFile tmpFile(QDir::tempPath() + QDir::separator() + "Spectacle.XXXXXX." + mimetype);
    tmpFile.setAutoRemove(false);
    tmpFile.setPermissions(QFile::ReadUser | QFile::WriteUser);

    if (tmpFile.open()) {
        if(!writeImage(&tmpFile, mimetype.toLatin1())) {
            emit errorMessage(i18n("Cannot save screenshot. Error while writing temporary local file."));
            return QUrl();
        }
        mTempFile = QUrl::fromLocalFile(tmpFile.fileName());
        return mTempFile;
    }

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

bool ExportManager::isFileExists(const QUrl &url)
{
    if (!(url.isValid())) {
        return false;
    }

    KIO::StatJob * existsJob = KIO::stat(url, KIO::StatJob::DestinationSide, 0);
    existsJob->exec();

    return (existsJob->error() == KJob::NoError);
}

// save slots

void ExportManager::doSave(const QUrl &url)
{
    if (mSavePixmap.isNull()) {
        emit errorMessage(i18n("Cannot save an empty screenshot image."));
        return;
    }

    QUrl savePath = url.isValid() ? url : getAutosaveFilename();
    if (save(savePath)) {
        QDir dir(savePath.path());
        dir.cdUp();
        setSaveLocation(dir.absolutePath());

        emit imageSaved(savePath);
    }
}

void ExportManager::doSaveAs(QWidget *parentWindow)
{
    QString selectedFilter;
    QStringList supportedFilters;
    QMimeDatabase db;

    const QUrl autoSavePath = getAutosaveFilename();
    const QMimeType mimeTypeForFilename = db.mimeTypeForUrl(autoSavePath);

    for (auto mimeTypeName: QImageWriter::supportedMimeTypes()) {
        QMimeType mimetype = db.mimeTypeForName(mimeTypeName);

        if (mimetype.preferredSuffix() != QStringLiteral("")) {
            QString filterString = mimetype.comment() + " (*." + mimetype.preferredSuffix() + ")";
            supportedFilters.append(filterString);
            if (mimetype == mimeTypeForFilename) {
                selectedFilter = supportedFilters.last();
            }
        }
    }

    QFileDialog dialog(parentWindow);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilters(supportedFilters);
    dialog.selectNameFilter(selectedFilter);
    dialog.setDirectoryUrl(autoSavePath);

    if (dialog.exec() == QFileDialog::Accepted) {
        const QUrl saveUrl = dialog.selectedUrls().first();
        if (saveUrl.isValid()) {
            if (save(saveUrl)) {
                emit imageSaved(saveUrl);
            }
        }
    }
}

// misc helpers

void ExportManager::doCopyToClipboard()
{
    QApplication::clipboard()->setPixmap(mSavePixmap, QClipboard::Clipboard);
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
