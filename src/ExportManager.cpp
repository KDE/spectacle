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
#include <QTemporaryDir>
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

#include "SpectacleConfig.h"

ExportManager::ExportManager(QObject *parent) :
    QObject(parent),
    mSavePixmap(QPixmap()),
    mTempFile(QUrl()),
    mTempDir(nullptr)
{}

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
        mUsedTempFileNames.append(mTempFile);
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

QString ExportManager::autoIncrementFilename(const QString &baseName, const QString &extension,
                                             FileNameAlreadyUsedCheck isFileNameUsed)
{
    if (!((this->*isFileNameUsed)(QUrl::fromUserInput(baseName + QLatin1Char('.') + extension)))) {
        return baseName + QLatin1Char('.') + extension;
    }

    QString fileNameFmt(baseName + QStringLiteral("-%1.") + extension);
    for (quint64 i = 1; i < std::numeric_limits<quint64>::max(); i++) {
        if (!((this->*isFileNameUsed)(QUrl::fromUserInput(fileNameFmt.arg(i))))) {
            return fileNameFmt.arg(i);
        }
    }

    // unlikely this will ever happen, but just in case we've run
    // out of numbers

    return fileNameFmt.arg(QStringLiteral("OVERFLOW-") + QString::number(qrand() % 10000));
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
    if (!(imageWriter.canWrite())) {
        emit errorMessage(i18n("QImageWriter cannot write image: %1", imageWriter.errorString()));
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

    if (!mTempDir) {
        mTempDir = new QTemporaryDir(QDir::tempPath() + QDir::separator() + QStringLiteral("Spectacle.XXXXXX"));
    }
    if (mTempDir && mTempDir->isValid()) {
        // create the temporary file itself with normal file name and also unique one for this session
        // supports the use-case of creating multiple screenshots in a row
        // and exporting them to the same destination e.g. via clipboard,
        // where the temp file name is used as filename suggestion
        const QString baseFileName = mTempDir->path() + QDir::separator() + makeAutosaveFilename();
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
        setSaveLocation(dir.absolutePath());

        emit imageSaved(savePath);
        if (notify) {
            emit forceNotify(savePath);
        }
    }
}

void ExportManager::doSaveAs(QWidget *parentWindow)
{
    QStringList supportedFilters;
    SpectacleConfig *config = SpectacleConfig::instance();

    // construct the supported mimetype list
    Q_FOREACH (auto mimeType, QImageWriter::supportedMimeTypes()) {
        supportedFilters.append(QString::fromUtf8(mimeType).trimmed());
    }

    // construct the file name
    QFileDialog dialog(parentWindow);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setDirectoryUrl(config->lastSaveAsLocation());
    dialog.selectFile(makeAutosaveFilename() + QStringLiteral(".png"));
    dialog.setDefaultSuffix(QStringLiteral(".png"));
    dialog.setMimeTypeFilters(supportedFilters);
    dialog.selectMimeTypeFilter(QStringLiteral("image/png"));

    // launch the dialog
    if (dialog.exec() == QFileDialog::Accepted) {
        const QUrl saveUrl = dialog.selectedUrls().first();
        if (saveUrl.isValid()) {
            if (save(saveUrl)) {
                emit imageSaved(saveUrl);
                config->setLastSaveAsLocation(saveUrl.adjusted(QUrl::RemoveFilename));
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
