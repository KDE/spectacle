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

#ifndef KSCORE_H
#define KSCORE_H

#include <QUrl>
#include <QFile>
#include <QTemporaryFile>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QDateTime>
#include <QImageWriter>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStandardPaths>
#include <QFileDialog>
#include <QPrinter>
#include <QPainter>
#include <QRect>
#include <QIcon>
#include <QDir>
#include <QDrag>
#include <QMimeData>
#include <QClipboard>
#include <QTimer>
#include <QMetaObject>
#include <QDebug>

#include <KLocalizedString>
#include <KJob>
#include <KRun>
#include <KService>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KWindowSystem>
#include <KMessageBox>
#include <KNotification>
#include <KIO/FileCopyJob>
#include <KIO/StatJob>

#include "Config.h"

#include "ExportManager.h"
#include "PlatformBackends/ImageGrabber.h"
#include "PlatformBackends/DummyImageGrabber.h"
#ifdef XCB_FOUND
#include "PlatformBackends/X11ImageGrabber.h"
#endif

#include "Gui/KSMainWindow.h"

class SpectacleCore : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString filename READ filename WRITE setFilename NOTIFY filenameChanged)
    Q_PROPERTY(ImageGrabber::GrabMode grabMode READ grabMode WRITE setGrabMode NOTIFY grabModeChanged)

    public:

    enum StartMode {
        GuiMode = 0,
        DBusMode = 1,
        BackgroundMode = 2
    };

    explicit SpectacleCore(StartMode startMode, ImageGrabber::GrabMode grabMode, QString &saveFileName,
                    qint64 delayMsec, bool notifyOnGrab, QObject *parent = 0);
    ~SpectacleCore();

    QString filename() const;
    void setFilename(const QString &filename);
    ImageGrabber::GrabMode grabMode() const;
    void setGrabMode(const ImageGrabber::GrabMode &grabMode);

    signals:

    void errorMessage(const QString errString);
    void allDone();
    void filenameChanged(QString filename);
    void grabModeChanged(ImageGrabber::GrabMode mode);
    void grabFailed();
    void imageSaved(const QString &savedAt);

    public slots:

    void takeNewScreenshot(const ImageGrabber::GrabMode &mode, const int &timeout, const bool &includePointer, const bool &includeDecorations);
    void showErrorMessage(const QString &errString);
    void screenshotUpdated(const QPixmap &pixmap);
    void screenshotFailed();
    void dbusStartAgent();
    void doStartDragAndDrop();
    void doNotify(const QUrl &savedAt);

    private:

    void initGui();

    ExportManager *mExportManager;
    StartMode     mStartMode;
    bool          mNotify;
    QString       mFileNameString;
    QUrl          mFileNameUrl;
    ImageGrabber *mImageGrabber;
    KSMainWindow *mMainWindow;
    bool          isGuiInited;
};

#endif // KSCORE_H
