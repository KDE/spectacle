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

#include "SpectacleCore.h"
#include "spectacle_core_debug.h"

#include "Config.h"
#include "PlatformBackends/DummyImageGrabber.h"
#ifdef XCB_FOUND
#include "PlatformBackends/X11ImageGrabber.h"
#endif
#include "PlatformBackends/KWinWaylandImageGrabber.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KNotification>
#include <KRun>
#include <KWindowSystem>

#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QDrag>
#include <QMimeData>
#include <QProcess>
#include <QTimer>

SpectacleCore::SpectacleCore(StartMode startMode, ImageGrabber::GrabMode grabMode, QString &saveFileName,
               qint64 delayMsec, bool notifyOnGrab, bool copyToClipboard, QObject *parent) :
    QObject(parent),
    mExportManager(ExportManager::instance()),
    mStartMode(startMode),
    mNotify(notifyOnGrab),
    mImageGrabber(nullptr),
    mMainWindow(nullptr),
    isGuiInited(false),
    copyToClipboard(copyToClipboard)
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("spectaclerc"));
    KConfigGroup guiConfig(config, "GuiConfig");

    if (!(saveFileName.isEmpty() || saveFileName.isNull())) {
        if (QDir::isRelativePath(saveFileName)) {
            saveFileName = QDir::current().absoluteFilePath(saveFileName);
        }
        setFilename(saveFileName);
    }

    // We might be using the XCB platform (with Xwayland) in a wayland session,
    // but the X11 grabber won't work in that case. So force the Wayland grabber
    // in Wayland sessions.
    if (KWindowSystem::isPlatformWayland() || qstrcmp(qgetenv("XDG_SESSION_TYPE"), "wayland") == 0) {
        mImageGrabber = new KWinWaylandImageGrabber;
    }
#ifdef XCB_FOUND
    else if (KWindowSystem::isPlatformX11()) {
        mImageGrabber = new X11ImageGrabber;
    }
#endif

    else {
        mImageGrabber = new DummyImageGrabber;
    }

    setGrabMode(grabMode);
    mImageGrabber->setCapturePointer(guiConfig.readEntry("includePointer", true));
    mImageGrabber->setCaptureDecorations(guiConfig.readEntry("includeDecorations", true));

    if ((!(mImageGrabber->onClickGrabSupported())) && (delayMsec < 0)) {
        delayMsec = 0;
    }

    //Reset last region if it should not be remembered across restarts
    SpectacleConfig* cfg = SpectacleConfig::instance();
    if(!cfg->alwaysRememberRegion()) {
        cfg->setCropRegion(QRect());
    }

    connect(mExportManager, &ExportManager::errorMessage, this, &SpectacleCore::showErrorMessage);
    connect(this, &SpectacleCore::errorMessage, this, &SpectacleCore::showErrorMessage);
    connect(mImageGrabber, &ImageGrabber::pixmapChanged, this, &SpectacleCore::screenshotUpdated);
    connect(mImageGrabber, &ImageGrabber::windowTitleChanged, mExportManager, &ExportManager::setWindowTitle);
    connect(mImageGrabber, &ImageGrabber::imageGrabFailed, this, &SpectacleCore::screenshotFailed);
    connect(mExportManager, &ExportManager::imageSaved, this, &SpectacleCore::doCopyPath);
    connect(mExportManager, &ExportManager::forceNotify, this, &SpectacleCore::doNotify);

    switch (startMode) {
    case DBusMode:
    default:
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
    delete mImageGrabber;
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

void SpectacleCore::setGrabMode(ImageGrabber::GrabMode grabMode)
{
    mImageGrabber->setGrabMode(grabMode);
    mExportManager->setGrabMode(grabMode);
}

// Slots

void SpectacleCore::dbusStartAgent()
{
    qApp->setQuitOnLastWindowClosed(true);
    if (!(mStartMode == GuiMode)) {
        mStartMode = GuiMode;
        initGui();
    } else {
        using Actions = SpectacleConfig::PrintKeyActionRunning;
        switch (SpectacleConfig::instance()->printKeyActionRunning()) {
            case Actions::TakeNewScreenshot:
                QTimer::singleShot(KWindowSystem::compositingActive() ? 200 : 50, mImageGrabber, &ImageGrabber::doImageGrab);
                break;
            case Actions::FocusWindow:
                KWindowSystem::forceActiveWindow(mMainWindow->winId());;
                break;
            case Actions::StartNewInstance:
                QProcess newInstance;
                newInstance.setProgram(QStringLiteral("spectacle"));
                newInstance.startDetached();
                break;
        }
    }
}

void SpectacleCore::takeNewScreenshot(const ImageGrabber::GrabMode &mode,
                               const int &timeout, const bool &includePointer, const bool &includeDecorations)
{
    setGrabMode(mode);
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
    qCDebug(SPECTACLE_CORE_LOG) << "ERROR: " << errString;

    if (mStartMode == GuiMode) {
        KMessageBox::error(nullptr, errString);
    }
}

void SpectacleCore::screenshotUpdated(const QPixmap &pixmap)
{
    mExportManager->setPixmap(pixmap);
    mExportManager->updatePixmapTimestamp();

    switch (mStartMode) {
    case BackgroundMode:
    case DBusMode:
    default:
        {
            if (mNotify) {
                connect(mExportManager, &ExportManager::imageSaved, this, &SpectacleCore::doNotify);
            }

            if (copyToClipboard) {
                mExportManager->doCopyToClipboard(mNotify);
            } else {
                QUrl savePath = (mStartMode == BackgroundMode && mFileNameUrl.isValid() && mFileNameUrl.isLocalFile()) ?
                    mFileNameUrl : QUrl();
                mExportManager->doSave(savePath);
            }

            // if we notify, we emit allDone only if the user either dismissed the notification or pressed
            // the "Open" button, otherwise the app closes before it can react to it.
            if (!mNotify) {
                emit allDone();
            }
        }
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
        emit allDone();
        return;
    case DBusMode:
    default:
        emit grabFailed();
        emit allDone();
        return;
    case GuiMode:
        mMainWindow->show();
    }
}

void SpectacleCore::doNotify(const QUrl &savedAt)
{
    KNotification *notify = new KNotification(QStringLiteral("newScreenshotSaved"));

    switch(mImageGrabber->grabMode()) {
    case ImageGrabber::GrabMode::FullScreen:
        notify->setTitle(i18nc("The entire screen area was captured, heading", "Full Screen Captured"));
        break;
    case ImageGrabber::GrabMode::CurrentScreen:
        notify->setTitle(i18nc("The current screen was captured, heading", "Current Screen Captured"));
        break;
    case ImageGrabber::GrabMode::ActiveWindow:
        notify->setTitle(i18nc("The active window was captured, heading", "Active Window Captured"));
        break;
    case ImageGrabber::GrabMode::WindowUnderCursor:
    case ImageGrabber::GrabMode::TransientWithParent:
        notify->setTitle(i18nc("The window under the mouse was captured, heading", "Window Under Cursor Captured"));
        break;
    case ImageGrabber::GrabMode::RectangularRegion:
        notify->setTitle(i18nc("A rectangular region was captured, heading", "Rectangular Region Captured"));
        break;
    case ImageGrabber::GrabMode::InvalidChoice:
    default:
        break;
    }

    const QString &path = savedAt.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).path();

    // a speaking message is prettier than a URL, special case for copy to clipboard and the default pictures location
    if (copyToClipboard) {
        notify->setText(i18n("A screenshot was saved to your clipboard."));
    } else if (path == QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)) {
        notify->setText(i18nc("Placeholder is filename", "A screenshot was saved as '%1' to your Pictures folder.", savedAt.fileName()));
    } else {
        notify->setText(i18n("A screenshot was saved as '%1' to '%2'.", savedAt.fileName(), path));
    }

    if (!copyToClipboard) {
        notify->setUrls({savedAt});

        notify->setDefaultAction(i18nc("Open the screenshot we just saved", "Open"));
        connect(notify, QOverload<uint>::of(&KNotification::activated), this, [this, savedAt](uint index) {
            if (index == 0) {
                new KRun(savedAt, nullptr);
                QTimer::singleShot(250, this, &SpectacleCore::allDone);
            }
        });
    }

    connect(notify, &QObject::destroyed, this, &SpectacleCore::allDone);

    notify->sendEvent();
}

void SpectacleCore::doCopyPath(const QUrl &savedAt)
{
    if (SpectacleConfig::instance()->copySaveLocationToClipboard()) {
        qApp->clipboard()->setText(savedAt.toLocalFile());
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
    mimeData->setData(QStringLiteral("application/x-kde-suggestedfilename"), QFile::encodeName(tempFile.fileName()));

    QDrag *dragHandler = new QDrag(this);
    dragHandler->setMimeData(mimeData);
    dragHandler->setPixmap(mExportManager->pixmap().scaled(256, 256, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    dragHandler->exec(Qt::CopyAction);
}

// Private

void SpectacleCore::initGui()
{
    if (!isGuiInited) {
        mMainWindow = new KSMainWindow(mImageGrabber->supportedModes(), mImageGrabber->onClickGrabSupported());

        connect(mMainWindow, &KSMainWindow::newScreenshotRequest, this, &SpectacleCore::takeNewScreenshot);
        connect(mMainWindow, &KSMainWindow::dragAndDropRequest, this, &SpectacleCore::doStartDragAndDrop);

        isGuiInited = true;
        QMetaObject::invokeMethod(mImageGrabber, "doImageGrab", Qt::QueuedConnection);
    }
}
