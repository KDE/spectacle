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

KScreenGenie::KScreenGenie(bool backgroundMode, ImageGrabber::GrabMode grabMode, QString &saveFileName, qint64 delayMsec, bool sendToClipboard, QObject *parent) :
    QObject(parent),
    mBackgroundMode(backgroundMode),
    mOverwriteOnSave(true),
    mBackgroundSendToClipboard(sendToClipboard),
    mLocalPixmap(QPixmap()),
    mScreenGenieGUI(nullptr)
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kscreengenierc");
    KConfigGroup guiConfig(config, "GuiConfig");

    if (!(saveFileName.isEmpty() || saveFileName.isNull())) {
        if (QDir::isRelativePath(saveFileName)) {
            saveFileName = QDir::current().absoluteFilePath(saveFileName);
        }
        setFilename(saveFileName);
    }

    mImageGrabber = new X11ImageGrabber();
    mImageGrabber->setGrabMode(grabMode);
    mImageGrabber->setCapturePointer(guiConfig.readEntry("includePointer", true));
    mImageGrabber->setCaptureDecorations(guiConfig.readEntry("includeDecorations", true));

    if ((!(mImageGrabber->onClickGrabSupported())) && (delayMsec < 0)) {
        delayMsec = 0;
    }

    connect(this, &KScreenGenie::errorMessage, this, &KScreenGenie::showErrorMessage);
    connect(mImageGrabber, &ImageGrabber::pixmapChanged, this, &KScreenGenie::screenshotUpdated);
    connect(mImageGrabber, &ImageGrabber::imageGrabFailed, this, &KScreenGenie::screenshotFailed);

    if (backgroundMode) {
        int msec = (KWindowSystem::compositingActive() ? 200 : 50) + delayMsec;
        QTimer::singleShot(msec, mImageGrabber, &ImageGrabber::doImageGrab);
        return;
    }

    // if we aren't in background mode, this would be a good time to
    // init the gui

    mScreenGenieGUI = new KScreenGenieGUI(mImageGrabber->onClickGrabSupported());

    connect(mScreenGenieGUI, &KScreenGenieGUI::newScreenshotRequest, this, &KScreenGenie::takeNewScreenshot);
    connect(mScreenGenieGUI, &KScreenGenieGUI::saveAndExit, this, &KScreenGenie::doAutoSave);
    connect(mScreenGenieGUI, &KScreenGenieGUI::saveAsClicked, this, &KScreenGenie::doGuiSaveAs);
    connect(mScreenGenieGUI, &KScreenGenieGUI::sendToKServiceRequest, this, &KScreenGenie::doSendToService);
    connect(mScreenGenieGUI, &KScreenGenieGUI::sendToOpenWithRequest, this, &KScreenGenie::doSendToOpenWith);
    connect(mScreenGenieGUI, &KScreenGenieGUI::sendToClipboardRequest, this, &KScreenGenie::doSendToClipboard);
    connect(mScreenGenieGUI, &KScreenGenieGUI::dragAndDropRequest, this, &KScreenGenie::doStartDragAndDrop);
    connect(mScreenGenieGUI, &KScreenGenieGUI::printRequest, this, &KScreenGenie::doPrint);

    QMetaObject::invokeMethod(mImageGrabber, "doImageGrab", Qt::QueuedConnection);
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

    if (timeout < 0) {
        mImageGrabber->doOnClickGrab();
        return;
    }

    const int msec = KWindowSystem::compositingActive() ? 200 : 50;
    QTimer::singleShot(timeout + msec, mImageGrabber, &ImageGrabber::doImageGrab);
}

void KScreenGenie::showErrorMessage(const QString err_string)
{
    qDebug() << "ERROR: " << err_string;

    if (!mBackgroundMode) {
        KMessageBox::error(0, err_string);
    }
}

void KScreenGenie::screenshotUpdated(const QPixmap pixmap)
{
    mLocalPixmap = pixmap;

    if (mBackgroundMode) {
        if (mBackgroundSendToClipboard) {
            qApp->clipboard()->setPixmap(pixmap);
            qDebug() << i18n("Copied image to clipboard");
        } else {
            doAutoSave();
        }
        emit allDone();
        return;
    } else {
#ifdef KIPI_FOUND
        doTempSaveForKipi();
        qDebug() << "KipiSave";
#endif
        mScreenGenieGUI->setScreenshotAndShow(pixmap);
    }
}

void KScreenGenie::screenshotFailed()
{
    if (mBackgroundMode) {
        qDebug() << i18n("Screenshot capture canceled or failed");
        emit allDone();
        return;
    }
    screenshotUpdated(QPixmap());
}

void KScreenGenie::doAutoSave()
{
    if (mLocalPixmap.isNull()) {
        emit errorMessage(i18n("Cannot save an empty screenshot image."));
        return;
    }

    QUrl savePath;

    if (mBackgroundMode && mFileNameUrl.isValid() && mFileNameUrl.isLocalFile()) {
        savePath = mFileNameUrl;
    } else {
        savePath = getAutoSaveFilename();
    }

    if (doSave(savePath)) {
        setSaveLocation(saveLocation());
        emit allDone();
        return;
    }
}

void KScreenGenie::doStartDragAndDrop()
{
    QMimeData *mimeData = new QMimeData;
    mimeData->setImageData(mLocalPixmap);
    mimeData->setData("application/x-kde-suggestedfilename", QFile::encodeName(makeTimestampFilename() + ".png"));

    QDrag *dragHandler = new QDrag(this);
    dragHandler->setMimeData(mimeData);
    dragHandler->setPixmap(mLocalPixmap.scaled(256, 256, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    dragHandler->start();
    dragHandler->deleteLater();
}

void KScreenGenie::doPrint(QPrinter *printer)
{
    QPainter painter;

    if (!(painter.begin(printer))) {
        emit errorMessage(i18n("Printing failed. The printer failed to initialize."));
        delete printer;
        return;
    }

    QRect devRect(0, 0, printer->width(), printer->height());
    QPixmap pixmap = mLocalPixmap.scaled(devRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QRect srcRect = pixmap.rect();
    srcRect.moveCenter(devRect.center());

    painter.drawPixmap(srcRect.topLeft(), pixmap);
    painter.end();

    delete printer;
    return;
}

void KScreenGenie::doGuiSaveAs()
{
    QString selectedFilter;
    QStringList supportedFilters;
    QMimeDatabase db;

    const QUrl autoSavePath = getAutoSaveFilename();
    const QMimeType mimeTypeForFilename = db.mimeTypeForUrl(autoSavePath);

    for (auto mimeTypeName: QImageWriter::supportedMimeTypes()) {
        QMimeType mimetype = db.mimeTypeForName(mimeTypeName);

        if (mimetype.preferredSuffix() != "") {
            QString filterString = mimetype.comment() + " (*." + mimetype.preferredSuffix() + ")";
            qDebug() << filterString;
            supportedFilters.append(filterString);
            if (mimetype == mimeTypeForFilename) {
                selectedFilter = supportedFilters.last();
            }
        }
    }

    QFileDialog dialog(mScreenGenieGUI);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilters(supportedFilters);
    dialog.selectNameFilter(selectedFilter);
    dialog.setDirectoryUrl(autoSavePath);

    if (dialog.exec() == QFileDialog::Accepted) {
        const QUrl saveUrl = dialog.selectedUrls().first();
        if (saveUrl.isValid()) {
            if (doSave(saveUrl)) {
                emit imageSaved();
            }
        }
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
        emit errorMessage(i18n("Cannot save screenshot. The save filename is invalid."));
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

// For Kipi

#ifdef KIPI_FOUND
void KScreenGenie::doTempSaveForKipi()
{
    if (!(mLocalPixmap.isNull())) {
        QDir tempDir = QDir::temp();
        QString tempFile = tempDir.absoluteFilePath("kscreengenie_kipi.png");

        localSave(QUrl::fromLocalFile(tempFile), "png");
    }
}
#endif
