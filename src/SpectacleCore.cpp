/*
 *  Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
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

#include "SpectacleCore.h"

SpectacleCore::SpectacleCore(StartMode startMode, ImageGrabber::GrabMode grabMode, QString &saveFileName,
               qint64 delayMsec, bool sendToClipboard, bool notifyOnGrab, QObject *parent) :
    QObject(parent),
    mExportManager(ExportManager::instance()),
    mStartMode(startMode),
    mNotify(notifyOnGrab),
    mOverwriteOnSave(true),
    mBackgroundSendToClipboard(sendToClipboard),
    mImageGrabber(nullptr),
    mMainWindow(nullptr),
    isGuiInited(false)
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("spectaclerc"));
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
#endif

    if (!mImageGrabber) {
        mImageGrabber = new DummyImageGrabber;
    }

    mImageGrabber->setGrabMode(grabMode);
    mImageGrabber->setCapturePointer(guiConfig.readEntry("includePointer", true));
    mImageGrabber->setCaptureDecorations(guiConfig.readEntry("includeDecorations", true));

    if ((!(mImageGrabber->onClickGrabSupported())) && (delayMsec < 0)) {
        delayMsec = 0;
    }

    connect(this, &SpectacleCore::errorMessage, this, &SpectacleCore::showErrorMessage);
    connect(mImageGrabber, &ImageGrabber::pixmapChanged, this, &SpectacleCore::screenshotUpdated);
    connect(mImageGrabber, &ImageGrabber::imageGrabFailed, this, &SpectacleCore::screenshotFailed);

    switch (startMode) {
    case DBusMode:
        break;
    case BackgroundMode: {
            int msec = (KWindowSystem::compositingActive() ? 200 : 50) + delayMsec;
            QTimer::singleShot(msec, mImageGrabber, &ImageGrabber::doImageGrab);
        }
        break;
    case GuiMode:
        initGui();
        break;
    }
}

SpectacleCore::~SpectacleCore()
{
    if (mMainWindow) {
        delete mMainWindow;
    }
}

// Q_PROPERTY stuff

QString SpectacleCore::filename() const
{
    return mFileNameString;
}

void SpectacleCore::setFilename(const QString &filename)
{
    mFileNameString = filename;
    mFileNameUrl = QUrl::fromUserInput(filename);
}

ImageGrabber::GrabMode SpectacleCore::grabMode() const
{
    return mImageGrabber->grabMode();
}

void SpectacleCore::setGrabMode(const ImageGrabber::GrabMode &grabMode)
{
    mImageGrabber->setGrabMode(grabMode);
}

bool SpectacleCore::overwriteOnSave() const
{
    return mOverwriteOnSave;
}

void SpectacleCore::setOverwriteOnSave(const bool &overwrite)
{
    mOverwriteOnSave = overwrite;
}

// Slots

void SpectacleCore::dbusStartAgent()
{
    qApp->setQuitOnLastWindowClosed(true);
    if (!(mStartMode == GuiMode)) {
        mStartMode = GuiMode;
        return initGui();
    }
}

void SpectacleCore::takeNewScreenshot(const ImageGrabber::GrabMode &mode,
                               const int &timeout, const bool &includePointer, const bool &includeDecorations)
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

void SpectacleCore::showErrorMessage(const QString &errString)
{
    qDebug() << "ERROR: " << errString;

    if (mStartMode == GuiMode) {
        KMessageBox::error(0, errString);
    }
}

void SpectacleCore::screenshotUpdated(const QPixmap &pixmap)
{
    mExportManager->setPixmap(pixmap);

    switch (mStartMode) {
    case BackgroundMode:
        if (mBackgroundSendToClipboard) {
            qApp->clipboard()->setPixmap(pixmap);
            qDebug() << i18n("Copied image to clipboard");
        }
    case DBusMode:
        doAutoSave();
        break;
    case GuiMode:
        mMainWindow->setScreenshotAndShow(pixmap);
    }
}

void SpectacleCore::screenshotFailed()
{
    switch (mStartMode) {
    case BackgroundMode:
        showErrorMessage(i18n("Screenshot capture canceled or failed"));
    case DBusMode:
        emit grabFailed();
        emit allDone();
        return;
    case GuiMode:
        mMainWindow->show();
    }
}

void SpectacleCore::doGuiSave()
{
    if (mExportManager->pixmap().isNull()) {
        emit errorMessage(i18n("Cannot save an empty screenshot image."));
        return;
    }

    QUrl savePath = mExportManager->getAutosaveFilename();
    if (mExportManager->doSave(savePath)) {
        emit imageSaved(savePath);
        emit imageSaved(savePath.toLocalFile());
    }
}

void SpectacleCore::doAutoSave()
{
    if (mExportManager->pixmap().isNull()) {
        emit errorMessage(i18n("Cannot save an empty screenshot image."));
        return;
    }

    QUrl savePath;

    if (mStartMode == BackgroundMode && mFileNameUrl.isValid() && mFileNameUrl.isLocalFile()) {
        savePath = mFileNameUrl;
    } else {
        savePath = mExportManager->getAutosaveFilename();
    }

    if (mExportManager->doSave(savePath)) {
        QDir dir(savePath.path());
        dir.cdUp();
        mExportManager->setSaveLocation(dir.absolutePath());

        if ((mStartMode == BackgroundMode || mStartMode == DBusMode) && mNotify) {
            KNotification *notify = new KNotification(QStringLiteral("newScreenshotSaved"));

            notify->setText(i18n("A new screenshot was captured and saved to %1", savePath.toLocalFile()));
            notify->setPixmap(QIcon::fromTheme(QStringLiteral("spectacle")).pixmap(QSize(32, 32)));
            notify->sendEvent();

            // unfortunately we can't quit just yet, emitting allDone right away
            // quits the application before the notification DBus message gets sent.
            // a token timeout seems to fix this though. Any better ideas?

            QTimer::singleShot(50, this, &SpectacleCore::allDone);
        } else {
            emit allDone();
        }

        return;
    }
}

void SpectacleCore::doStartDragAndDrop()
{
    QUrl tempFile = mExportManager->tempSave();
    if (!tempFile.isValid()) {
        return;
    }

    QMimeData *mimeData = new QMimeData;
    mimeData->setUrls(QList<QUrl> { tempFile });
    mimeData->setImageData(mExportManager->pixmap());
    mimeData->setData(QStringLiteral("application/x-kde-suggestedfilename"), QFile::encodeName(tempFile.fileName()));

    QDrag *dragHandler = new QDrag(this);
    dragHandler->setMimeData(mimeData);
    dragHandler->setPixmap(mExportManager->pixmap().scaled(256, 256, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    dragHandler->exec();
}

void SpectacleCore::doPrint(QPrinter *printer)
{
    QPainter painter;

    if (!(painter.begin(printer))) {
        emit errorMessage(i18n("Printing failed. The printer failed to initialize."));
        delete printer;
        return;
    }

    QRect devRect(0, 0, printer->width(), printer->height());
    QPixmap pixmap = mExportManager->pixmap().scaled(devRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QRect srcRect = pixmap.rect();
    srcRect.moveCenter(devRect.center());

    painter.drawPixmap(srcRect.topLeft(), pixmap);
    painter.end();

    delete printer;
    return;
}

void SpectacleCore::doGuiSaveAs()
{
    QString selectedFilter;
    QStringList supportedFilters;
    QMimeDatabase db;

    const QUrl autoSavePath = mExportManager->getAutosaveFilename();
    const QMimeType mimeTypeForFilename = db.mimeTypeForUrl(autoSavePath);

    for (auto mimeTypeName: QImageWriter::supportedMimeTypes()) {
        QMimeType mimetype = db.mimeTypeForName(mimeTypeName);

        if (mimetype.preferredSuffix() != QLatin1String("")) {
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
            if (mExportManager->doSave(saveUrl)) {
                emit imageSaved(saveUrl);
                emit imageSaved(saveUrl.toLocalFile());
            }
        }
    }
}

void SpectacleCore::doSendToService(KService::Ptr service)
{
    QUrl tempFile;
    QList<QUrl> tempFileList;

    tempFile = mExportManager->tempSave();
    if (!tempFile.isValid()) {
        emit errorMessage(i18n("Cannot send screenshot to the application"));
        return;
    }

    tempFileList.append(tempFile);
    KRun::runService(*service, tempFileList, mMainWindow, true);
}

void SpectacleCore::doSendToOpenWith()
{
    QUrl tempFile;
    QList<QUrl> tempFileList;

    tempFile = mExportManager->tempSave();
    if (!tempFile.isValid()) {
        emit errorMessage(i18n("Cannot send screenshot to the application"));
        return;
    }

    tempFileList.append(tempFile);
    KRun::displayOpenWithDialog(tempFileList, mMainWindow, true);
}

void SpectacleCore::doSendToClipboard()
{
    QApplication::clipboard()->setPixmap(mExportManager->pixmap());
}

// Private

void SpectacleCore::initGui()
{
    if (!isGuiInited) {
        mMainWindow = new KSMainWindow(mImageGrabber->onClickGrabSupported());

        connect(mMainWindow, &KSMainWindow::newScreenshotRequest, this, &SpectacleCore::takeNewScreenshot);
        connect(mMainWindow, &KSMainWindow::save, this, &SpectacleCore::doGuiSave);
        connect(mMainWindow, &KSMainWindow::saveAndExit, this, &SpectacleCore::doAutoSave);
        connect(mMainWindow, &KSMainWindow::saveAsClicked, this, &SpectacleCore::doGuiSaveAs);
        connect(mMainWindow, &KSMainWindow::sendToKServiceRequest, this, &SpectacleCore::doSendToService);
        connect(mMainWindow, &KSMainWindow::sendToOpenWithRequest, this, &SpectacleCore::doSendToOpenWith);
        connect(mMainWindow, &KSMainWindow::sendToClipboardRequest, this, &SpectacleCore::doSendToClipboard);
        connect(mMainWindow, &KSMainWindow::dragAndDropRequest, this, &SpectacleCore::doStartDragAndDrop);
        connect(mMainWindow, &KSMainWindow::printRequest, this, &SpectacleCore::doPrint);

        connect(this, static_cast<void (SpectacleCore::*)(QUrl)>(&SpectacleCore::imageSaved),
                mMainWindow, &KSMainWindow::setScreenshotWindowTitle);

        isGuiInited = true;
        QMetaObject::invokeMethod(mImageGrabber, "doImageGrab", Qt::QueuedConnection);
    }
}
