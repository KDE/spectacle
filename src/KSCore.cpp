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

#include "KSCore.h"

KSCore::KSCore(bool backgroundMode, ImageGrabber::GrabMode grabMode, QString &saveFileName, qint64 delayMsec, bool sendToClipboard, bool notifyOnGrab, QObject *parent) :
    QObject(parent),
    mBackgroundMode(backgroundMode),
    mNotify(notifyOnGrab),
    mOverwriteOnSave(true),
    mBackgroundSendToClipboard(sendToClipboard),
    mLocalPixmap(QPixmap()),
    mImageGrabber(nullptr),
    mMainWindow(nullptr)
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kscreengenierc");
    KConfigGroup guiConfig(config, "GuiConfig");

    if (!(saveFileName.isEmpty() || saveFileName.isNull())) {
        if (QDir::isRelativePath(saveFileName)) {
            saveFileName = QDir::current().absoluteFilePath(saveFileName);
        }
        setFilename(saveFileName);
    }

#ifdef XCB_FOUND
    if (qApp->platformName() == QStringLiteral("xcb")) {
        mImageGrabber = new X11ImageGrabber;
    }
#endif // XCB_FOUND

    if (!mImageGrabber) {
        mImageGrabber = new DummyImageGrabber;
    }

    mImageGrabber->setGrabMode(grabMode);
    mImageGrabber->setCapturePointer(guiConfig.readEntry("includePointer", true));
    mImageGrabber->setCaptureDecorations(guiConfig.readEntry("includeDecorations", true));

    if ((!(mImageGrabber->onClickGrabSupported())) && (delayMsec < 0)) {
        delayMsec = 0;
    }

    connect(this, &KSCore::errorMessage, this, &KSCore::showErrorMessage);
    connect(mImageGrabber, &ImageGrabber::pixmapChanged, this, &KSCore::screenshotUpdated);
    connect(mImageGrabber, &ImageGrabber::imageGrabFailed, this, &KSCore::screenshotFailed);

    if (backgroundMode) {
        int msec = (KWindowSystem::compositingActive() ? 200 : 50) + delayMsec;
        QTimer::singleShot(msec, mImageGrabber, &ImageGrabber::doImageGrab);
        return;
    }

    // if we aren't in background mode, this would be a good time to
    // init the gui

    mMainWindow = new KSMainWindow(mImageGrabber->onClickGrabSupported());

    connect(mMainWindow, &KSMainWindow::newScreenshotRequest, this, &KSCore::takeNewScreenshot);
    connect(mMainWindow, &KSMainWindow::saveAndExit, this, &KSCore::doAutoSave);
    connect(mMainWindow, &KSMainWindow::saveAsClicked, this, &KSCore::doGuiSaveAs);
    connect(mMainWindow, &KSMainWindow::sendToKServiceRequest, this, &KSCore::doSendToService);
    connect(mMainWindow, &KSMainWindow::sendToOpenWithRequest, this, &KSCore::doSendToOpenWith);
    connect(mMainWindow, &KSMainWindow::sendToClipboardRequest, this, &KSCore::doSendToClipboard);
    connect(mMainWindow, &KSMainWindow::dragAndDropRequest, this, &KSCore::doStartDragAndDrop);
    connect(mMainWindow, &KSMainWindow::printRequest, this, &KSCore::doPrint);

    connect(this, &KSCore::imageSaved, mMainWindow, &KSMainWindow::setScreenshotWindowTitle);
    QMetaObject::invokeMethod(mImageGrabber, "doImageGrab", Qt::QueuedConnection);
}

KSCore::~KSCore()
{
    if (mMainWindow) {
        delete mMainWindow;
    }
}

// Q_PROPERTY stuff

QString KSCore::filename() const
{
    return mFileNameString;
}

void KSCore::setFilename(const QString &filename)
{
    mFileNameString = filename;
    mFileNameUrl = QUrl::fromUserInput(filename);
}

ImageGrabber::GrabMode KSCore::grabMode() const
{
    return mImageGrabber->grabMode();
}

void KSCore::setGrabMode(const ImageGrabber::GrabMode &grabMode)
{
    mImageGrabber->setGrabMode(grabMode);
}

bool KSCore::overwriteOnSave() const
{
    return mOverwriteOnSave;
}

void KSCore::setOverwriteOnSave(const bool &overwrite)
{
    mOverwriteOnSave = overwrite;
}

QString KSCore::saveLocation() const
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kscreengenierc");
    KConfigGroup generalConfig = KConfigGroup(config, "General");

    QString savePath = generalConfig.readPathEntry(
                "default-save-location", QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    if (savePath.isEmpty() || savePath.isNull()) {
        savePath = QDir::homePath();
    }
    savePath = QDir::cleanPath(savePath);

    QDir savePathDir(savePath);
    if (!(savePathDir.exists())) {
        savePathDir.mkpath(".");
        generalConfig.writePathEntry("last-saved-to", savePath);
    }

    return savePath;
}

void KSCore::setSaveLocation(const QString &savePath)
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kscreengenierc");
    KConfigGroup generalConfig = KConfigGroup(config, "General");

    generalConfig.writePathEntry("last-saved-to", savePath);
}

// Slots

void KSCore::takeNewScreenshot(const ImageGrabber::GrabMode &mode, const int &timeout, const bool &includePointer, const bool &includeDecorations)
{
    mImageGrabber->setGrabMode(mode);
    mImageGrabber->setCapturePointer(includePointer);
    mImageGrabber->setCaptureDecorations(includeDecorations);

    if (timeout < 0) {
        mImageGrabber->doOnClickGrab();
        return;
    }

    // when compositing is enabled, we need to give it enough time for the window
    // to disappear and all the effects are complete before we take the shot. there's
    // no way of knowing how long the disappearing effects take, but as per default
    // settings (and unless the user has set an extremely slow effect), 200
    // milliseconds is a good amount of wait time.

    const int msec = KWindowSystem::compositingActive() ? 200 : 50;
    QTimer::singleShot(timeout + msec, mImageGrabber, &ImageGrabber::doImageGrab);
}

void KSCore::showErrorMessage(const QString &errString)
{
    qDebug() << "ERROR: " << errString;

    if (!mBackgroundMode) {
        KMessageBox::error(0, errString);
    }
}

void KSCore::screenshotUpdated(const QPixmap &pixmap)
{
    mLocalPixmap = pixmap;

    if (mBackgroundMode) {
        if (mBackgroundSendToClipboard) {
            qApp->clipboard()->setPixmap(pixmap);
            qDebug() << i18n("Copied image to clipboard");
        } else {
            doAutoSave();
        }
    } else {
        mMainWindow->setScreenshotAndShow(pixmap);
        tempFileSave();
    }
}

void KSCore::screenshotFailed()
{
    if (mBackgroundMode) {
        qDebug() << i18n("Screenshot capture canceled or failed");
        emit allDone();
        return;
    }

    mMainWindow->show();
}

void KSCore::doAutoSave()
{
    if (mLocalPixmap.isNull()) {
        emit errorMessage(i18n("Cannot save an empty screenshot image."));
        return;
    }

    QUrl savePath;

    if (mBackgroundMode && mFileNameUrl.isValid() && mFileNameUrl.isLocalFile()) {
        savePath = mFileNameUrl;
    } else {
        savePath = getAutosaveFilename();
    }

    if (doSave(savePath)) {
        QDir dir(savePath.path());
        dir.cdUp();
        setSaveLocation(dir.absolutePath());

        if (mBackgroundMode && mNotify) {
            KNotification *notify = new KNotification("newScreenshotSaved");

            notify->setText(i18n("A new screenshot was captured and saved to %1", savePath.toLocalFile()));
            notify->setPixmap(QIcon::fromTheme("ksnapshot").pixmap(QSize(32, 32)));
            notify->sendEvent();

            // unfortunately we can't quit just yet, emitting allDone right away
            // quits the application before the notification DBus message gets sent.
            // a token timeout seems to fix this though. Any better ideas?

            QTimer::singleShot(50, this, &KSCore::allDone);
        } else {
            emit allDone();
        }

        return;
    }
}

void KSCore::doStartDragAndDrop()
{
    QMimeData *mimeData = new QMimeData;
    mimeData->setUrls(QList<QUrl> { getTempSaveFilename() });
    mimeData->setImageData(mLocalPixmap);
    mimeData->setData("application/x-kde-suggestedfilename", QFile::encodeName(makeAutosaveFilename() + ".png"));

    QDrag *dragHandler = new QDrag(this);
    dragHandler->setMimeData(mimeData);
    dragHandler->setPixmap(mLocalPixmap.scaled(256, 256, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    dragHandler->start();
    dragHandler->deleteLater();
}

void KSCore::doPrint(QPrinter *printer)
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

void KSCore::doGuiSaveAs()
{
    QString selectedFilter;
    QStringList supportedFilters;
    QMimeDatabase db;

    const QUrl autoSavePath = getAutosaveFilename();
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

    QFileDialog dialog(mMainWindow);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilters(supportedFilters);
    dialog.selectNameFilter(selectedFilter);
    dialog.setDirectoryUrl(autoSavePath);

    if (dialog.exec() == QFileDialog::Accepted) {
        const QUrl saveUrl = dialog.selectedUrls().first();
        if (saveUrl.isValid()) {
            if (doSave(saveUrl)) {
                emit imageSaved(saveUrl);
            }
        }
    }
}

void KSCore::doSendToService(KService::Ptr service)
{
    QUrl tempFile;
    QList<QUrl> tempFileList;

    tempFile = getTempSaveFilename();
    if (!tempFile.isValid()) {
        emit errorMessage(i18n("Cannot send screenshot to the application"));
        return;
    }

    tempFileList.append(tempFile);
    KRun::runService(*service, tempFileList, mMainWindow, true);
}

void KSCore::doSendToOpenWith()
{
    QUrl tempFile;
    QList<QUrl> tempFileList;

    tempFile = getTempSaveFilename();
    if (!tempFile.isValid()) {
        emit errorMessage(i18n("Cannot send screenshot to the application"));
        return;
    }

    tempFileList.append(tempFile);
    KRun::displayOpenWithDialog(tempFileList, mMainWindow, true);
}

void KSCore::doSendToClipboard()
{
    QApplication::clipboard()->setPixmap(mLocalPixmap);
}

// Private

QUrl KSCore::getAutosaveFilename()
{
    const QString baseDir = saveLocation();
    const QDir baseDirPath(baseDir);
    const QString filename = makeAutosaveFilename();
    const QString fullpath = autoIncrementFilename(baseDirPath.filePath(filename), "png");

    const QUrl fileNameUrl = QUrl::fromUserInput(fullpath);
    if (fileNameUrl.isValid()) {
        return fileNameUrl;
    } else {
        return QUrl();
    }
}

QString KSCore::makeAutosaveFilename()
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kscreengenierc");
    KConfigGroup generalConfig = KConfigGroup(config, "General");

    const QDateTime timestamp = QDateTime::currentDateTime();
    QString baseName = generalConfig.readEntry("save-filename-format", "Screenshot_%Y%M%D_%H%m%S");

    return baseName.replace("%Y", timestamp.toString("yyyy"))
                   .replace("%y", timestamp.toString("yy"))
                   .replace("%M", timestamp.toString("MM"))
                   .replace("%D", timestamp.toString("dd"))
                   .replace("%H", timestamp.toString("hh"))
                   .replace("%m", timestamp.toString("mm"))
                   .replace("%S", timestamp.toString("ss"));
}

QString KSCore::autoIncrementFilename(const QString &baseName, const QString &extension)
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

QString KSCore::makeSaveMimetype(const QUrl &url)
{
    QMimeDatabase mimedb;
    QString type = mimedb.mimeTypeForUrl(url).preferredSuffix();

    if (type.isEmpty()) {
        return QString("png");
    }
    return type;
}

bool KSCore::writeImage(QIODevice *device, const QByteArray &format)
{
    QImageWriter imageWriter(device, format);
    if (!(imageWriter.canWrite())) {
        emit errorMessage(i18n("QImageWriter cannot write image: ") + imageWriter.errorString());
        return false;
    }

    return imageWriter.write(mLocalPixmap.toImage());
}

bool KSCore::localSave(const QUrl &url, const QString &mimetype)
{
    QFile outputFile(url.toLocalFile());

    outputFile.open(QFile::WriteOnly);
    if(!writeImage(&outputFile, mimetype.toLatin1())) {
        emit errorMessage(i18n("Cannot save screenshot. Error while writing file."));
        return false;
    }
    return true;
}

bool KSCore::remoteSave(const QUrl &url, const QString &mimetype)
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

QUrl KSCore::getTempSaveFilename() const
{
    QDir tempDir = QDir::temp();
    return QUrl::fromLocalFile(tempDir.absoluteFilePath("KSTempScreenshot.png"));
}

bool KSCore::tempFileSave()
{
    if (!(mLocalPixmap.isNull())) {
        return localSave(getTempSaveFilename(), "png");
    }
    return false;
}

QUrl KSCore::tempFileSave(const QString &mimetype)
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

bool KSCore::doSave(const QUrl &url)
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

bool KSCore::isFileExists(const QUrl &url)
{
    if (!(url.isValid())) {
        return false;
    }

    KIO::StatJob * existsJob = KIO::stat(url, KIO::StatJob::DestinationSide, 0);
    existsJob->exec();

    return (existsJob->error() == KJob::NoError);
}
