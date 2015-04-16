/*
 *  Copyright (C) 2015 Boudhayan Gupta <me@BaloneyGeek.com>
 *
 *  Includes code from ksnapshot.cpp, part of KSnapshot. Copyright notices
 *  reproduced below:
 *
 *  Copyright (C) 1997-2008 Richard J. Moore <rich@kde.org>
 *  Copyright (C) 2000 Matthias Ettrich <ettrich@troll.no>
 *  Copyright (C) 2002 Aaron J. Seigo <aseigo@kde.org>
 *  Copyright (C) 2003 Nadeem Hasan <nhasan@kde.org>
 *  Copyright (C) 2004 Bernd Brandstetter <bbrand@freenet.de>
 *  Copyright (C) 2006 Urs Wolfer <uwolfer @ kde.org>
 *  Copyright (C) 2010 Martin Gräßlin <kde@martin-graesslin.com>
 *  Copyright (C) 2010, 2011 Pau Garcia i Quiles <pgquiles@elpauer.org>
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

#include "KScreenGenie.h"

KScreenGenie::KScreenGenie(bool background, bool startEditor, ImageGrabber::GrabMode grabMode, QObject *parent) :
    QObject(parent),
    mBackgroundMode(background),
    mOverwriteOnSave(true),
    mStartEditor(startEditor),
    mLocalPixmap(QPixmap()),
    mScreenGenieGUI(nullptr)
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kscreengenierc");
    KConfigGroup guiConfig(config, "GuiConfig");

    mImageGrabber = new X11ImageGrabber();
    mImageGrabber->setGrabMode(grabMode);
    mImageGrabber->setCapturePointer(guiConfig.readEntry("includePointer", true));
    mImageGrabber->setCaptureDecorations(guiConfig.readEntry("includeDecorations", true));

    connect(this, &KScreenGenie::errorMessage, this, &KScreenGenie::showErrorMessage);
    connect(mImageGrabber, &ImageGrabber::pixmapChanged, this, &KScreenGenie::screenshotUpdated);
    connect(mImageGrabber, &ImageGrabber::imageGrabFailed, this, &KScreenGenie::screenshotFailed);

    const int msec = KWindowSystem::compositingActive() ? 200 : 50;
    QTimer::singleShot(msec, mImageGrabber, &ImageGrabber::doImageGrab);

    // if we aren't in background mode, this would be a good time to
    // init the gui

    if (!(background)) {
        mScreenGenieGUI = new KScreenGenieGUI;

        connect(mScreenGenieGUI, &KScreenGenieGUI::newScreenshotRequest, this, &KScreenGenie::takeNewScreenshot);
        connect(mScreenGenieGUI, &KScreenGenieGUI::saveAndExit, this, &KScreenGenie::doAutoSave);
        connect(mScreenGenieGUI, &KScreenGenieGUI::saveAsClicked, this, &KScreenGenie::doGuiSaveAs);
        connect(mScreenGenieGUI, &KScreenGenieGUI::sendToServiceRequest, this, &KScreenGenie::doSendToService);
        connect(mScreenGenieGUI, &KScreenGenieGUI::sendToOpenWithRequest, this, &KScreenGenie::doSendToOpenWith);
        connect(mScreenGenieGUI, &KScreenGenieGUI::sendToClipboardRequest, this, &KScreenGenie::doSendToClipboard);
    }
}

KScreenGenie::~KScreenGenie()
{
    if (mScreenGenieGUI) {
        delete mScreenGenieGUI;
    }
}

// Q_PROPERTY stuff

QString KScreenGenie::filename() const
{
    return mFileNameString;
}

void KScreenGenie::setFilename(const QString &filename)
{
    mFileNameString = filename;
    mFileNameUrl = QUrl::fromUserInput(filename);
}

ImageGrabber::GrabMode KScreenGenie::grabMode() const
{
    return mImageGrabber->grabMode();
}

void KScreenGenie::setGrabMode(const ImageGrabber::GrabMode grabMode)
{
    mImageGrabber->setGrabMode(grabMode);
}

bool KScreenGenie::overwriteOnSave() const
{
    return mOverwriteOnSave;
}

void KScreenGenie::setOverwriteOnSave(const bool overwrite)
{
    mOverwriteOnSave = overwrite;
}

QString KScreenGenie::saveLocation() const
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kscreengenierc");
    KConfigGroup generalConfig = KConfigGroup(config, "General");

    QString savePath = generalConfig.readPathEntry(
                "last-saved-to", QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    if (savePath.isEmpty() || savePath.isNull()) {
        savePath = QDir::homePath();
    }
    savePath = QDir::cleanPath(savePath);

    QDir savePathDir(savePath);
    if (!(savePathDir.exists())) {
        savePathDir.mkpath(".");
    }

    return savePath;
}

void KScreenGenie::setSaveLocation(const QString &savePath)
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kscreengenierc");
    KConfigGroup generalConfig = KConfigGroup(config, "General");

    generalConfig.writePathEntry("last-saved-to", savePath);
}

// Slots

void KScreenGenie::takeNewScreenshot(ImageGrabber::GrabMode mode, int timeout, bool includePointer, bool includeDecorations)
{
    mImageGrabber->setGrabMode(mode);
    mImageGrabber->setCapturePointer(includePointer);
    mImageGrabber->setCaptureDecorations(includeDecorations);

    const int msec = KWindowSystem::compositingActive() ? 200 : 50;
    QTimer::singleShot(timeout + msec, mImageGrabber, &ImageGrabber::doImageGrab);
}

void KScreenGenie::showErrorMessage(const QString err_string)
{
    qDebug() << "ERROR: " << err_string;

    if (!mBackgroundMode) {
        QErrorMessage messageDialog;
        messageDialog.showMessage(err_string);
    }
}

void KScreenGenie::screenshotUpdated(const QPixmap pixmap)
{
    mLocalPixmap = pixmap;

    if (mBackgroundMode) {
        doAutoSave();
        emit allDone();
        return;
    } else {
        mScreenGenieGUI->setScreenshotAndShow(pixmap);
    }
}

void KScreenGenie::screenshotFailed()
{
    screenshotUpdated(QPixmap());
}

void KScreenGenie::doAutoSave()
{
    if (mLocalPixmap.isNull()) {
        emit errorMessage(i18n("Cannot save an empty screenshot image."));
        return;
    }

    const QUrl savePath = getAutoSaveFilename();

    if (doSave(savePath)) {
        setSaveLocation(saveLocation());
        emit allDone();
        return;
    }
}

void KScreenGenie::doGuiSaveAs()
{
    QString *selectedFilter;
    QStringList supportedFilters;
    QMimeDatabase db;

    const QUrl autoSavePath = getAutoSaveFilename();
    const QMimeType mimeTypeForFilename = db.mimeTypeForUrl(autoSavePath);

    for (auto mimeTypeName: QImageWriter::supportedMimeTypes()) {
        QMimeType mimetype = db.mimeTypeForName(mimeTypeName);

        if (mimetype.filterString() != "") {
            supportedFilters.append(mimetype.filterString());
            if (mimetype == mimeTypeForFilename) {
                selectedFilter = &supportedFilters.last();
            }
        }
    }

    const QUrl saveUrl = QFileDialog::getSaveFileUrl(
                mScreenGenieGUI, i18n("Save Screenshot As"), autoSavePath, supportedFilters.join(";;"), selectedFilter);
    if (!saveUrl.isValid()) {
        return;
    }

    if (doSave(saveUrl)) {
        emit imageSaved();
    }
}

void KScreenGenie::doSendToService(KService::Ptr service)
{
    QUrl tempFile;
    QList<QUrl> tempFileList;

    tempFile = tempFileSave();
    if (!tempFile.isValid()) {
        emit errorMessage(i18n("Cannot send screenshot to the application"));
        return;
    }

    tempFileList.append(tempFile);
    KRun::runService(*service, tempFileList, mScreenGenieGUI, true);
}

void KScreenGenie::doSendToOpenWith()
{
    QUrl tempFile;
    QList<QUrl> tempFileList;

    tempFile = tempFileSave();
    if (!tempFile.isValid()) {
        emit errorMessage(i18n("Cannot send screenshot to the application"));
        return;
    }

    tempFileList.append(tempFile);
    KRun::displayOpenWithDialog(tempFileList, mScreenGenieGUI, true);
}

void KScreenGenie::doSendToClipboard()
{
    QApplication::clipboard()->setPixmap(mLocalPixmap);
}

// Private

QUrl KScreenGenie::getAutoSaveFilename()
{
    QString baseDir = saveLocation();
    QDir baseDirPath(baseDir);
    QString filename = makeTimestampFilename() + ".png";
    QString fullpath = baseDirPath.filePath(filename);

    QUrl fileNameUrl = QUrl::fromLocalFile(fullpath);
    if (fileNameUrl.isValid()) {
        return fileNameUrl;
    } else {
        return QUrl();
    }
}

QString KScreenGenie::makeTimestampFilename()
{
    QString baseTime = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss"));
    QString baseName("Screenshot_");

    return (baseName + baseTime);
}

QString KScreenGenie::makeSaveMimetype(const QUrl url)
{
    QMimeDatabase mimedb;
    QString type = mimedb.mimeTypeForUrl(url).preferredSuffix();

    if (type.isEmpty()) {
        return QString("png");
    }
    return type;
}

bool KScreenGenie::writeImage(QIODevice *device, const QByteArray &format)
{
    QImageWriter imageWriter(device, format);
    if (!(imageWriter.canWrite())) {
        emit errorMessage(i18n("QImageWriter cannot write image: ") + imageWriter.errorString());
        return false;
    }

    return imageWriter.write(mLocalPixmap.toImage());
}

bool KScreenGenie::localSave(const QUrl url, const QString mimetype)
{
    QFile outputFile(url.toLocalFile());

    outputFile.open(QFile::WriteOnly);
    if(!writeImage(&outputFile, mimetype.toLatin1())) {
        emit errorMessage(i18n("Cannot save screenshot. Error while writing file."));
        return false;
    }
    return true;
}

bool KScreenGenie::remoteSave(const QUrl url, const QString mimetype)
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
            emit errorMessage("Unable to save image. Could not upload file to remote location.");
            return false;
        }
        return true;
    }

    return false;
}

QUrl KScreenGenie::tempFileSave(const QString mimetype)
{
    QTemporaryFile tmpFile;
    tmpFile.setAutoRemove(false);

    if (tmpFile.open()) {
        if(!writeImage(&tmpFile, mimetype.toLatin1())) {
            emit errorMessage(i18n("Cannot save screenshot. Error while writing temporary local file."));
            return QUrl();
        }
        return QUrl::fromLocalFile(tmpFile.fileName());
    }

    return QUrl();
}

bool KScreenGenie::doSave(const QUrl url)
{
    if (!(url.isValid())) {
        emit errorMessage(i18n("Canot save screenshot. The save filename is invalid."));
        return false;
    }

    if (isFileExists(url) && (mOverwriteOnSave == false)) {
        emit errorMessage((i18n("Cannot save screenshot. The file already exists.")));
        return false;
    }

    QString mimetype = makeSaveMimetype(url);
    if (url.isLocalFile()) {
        return localSave(url, mimetype);
    }
    return remoteSave(url, mimetype);
}

bool KScreenGenie::isFileExists(const QUrl url)
{
    if (!(url.isValid())) {
        return false;
    }

    KIO::StatJob * existsJob = KIO::stat(url, KIO::StatJob::DestinationSide, 0);
    existsJob->exec();

    return (existsJob->error() == KJob::NoError);
}
