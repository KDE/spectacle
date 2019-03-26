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

#include <QObject>

#include "ExportManager.h"
#include "Gui/KSMainWindow.h"
#include "PlatformBackends/ImageGrabber.h"

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
                    qint64 delayMsec, bool notifyOnGrab, bool copyToClipboard, QObject *parent = nullptr);
    ~SpectacleCore();

    QString filename() const;
    void setFilename(const QString &filename);
    ImageGrabber::GrabMode grabMode() const;
    void setGrabMode(ImageGrabber::GrabMode grabMode);

    Q_SIGNALS:

    void errorMessage(const QString &errString);
    void allDone();
    void filenameChanged(const QString &filename);
    void grabModeChanged(ImageGrabber::GrabMode mode);
    void grabFailed();

    public Q_SLOTS:

    void takeNewScreenshot(const ImageGrabber::GrabMode &mode, const int &timeout, const bool &includePointer, const bool &includeDecorations);
    void showErrorMessage(const QString &errString);
    void screenshotUpdated(const QPixmap &pixmap);
    void screenshotFailed();
    void dbusStartAgent();
    void doStartDragAndDrop();
    void doNotify(const QUrl &savedAt);
    void doCopyPath(const QUrl &savedAt);

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
    bool          copyToClipboard;
};

#endif // KSCORE_H
