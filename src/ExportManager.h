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

#ifndef EXPORTMANAGER_H
#define EXPORTMANAGER_H

#include <QObject>
#include <QIODevice>
#include <QMap>
#include <QPrinter>
#include <QPixmap>
#include <QDateTime>
#include <QUrl>

#include <KLocalizedString>

#include "PlatformBackends/ImageGrabber.h"

class QTemporaryDir;

class ExportManager : public QObject
{
    Q_OBJECT

    // singleton-ize the class

    public:

    static ExportManager* instance();

    private:

    explicit ExportManager(QObject *parent = nullptr);
    virtual ~ExportManager();

    ExportManager(ExportManager const&) = delete;
    void operator= (ExportManager const&) = delete;

    // now the usual stuff

    public:

    Q_PROPERTY(QPixmap pixmap READ pixmap WRITE setPixmap NOTIFY pixmapChanged)
    Q_PROPERTY(QString windowTitle READ windowTitle WRITE setWindowTitle)
    Q_PROPERTY(ImageGrabber::GrabMode grabMode READ grabMode WRITE setGrabMode)

    QString defaultSaveLocation() const;
    bool isFileExists(const QUrl &url) const;
    void setPixmap(const QPixmap &pixmap);
    QPixmap pixmap() const;
    void updatePixmapTimestamp();
    void setWindowTitle(const QString &windowTitle);
    QString windowTitle() const;
    ImageGrabber::GrabMode grabMode() const;
    void setGrabMode(const ImageGrabber::GrabMode &grabMode);
    QString formatFilename(const QString &nameTemplate);

    static const QMap<QString, KLocalizedString> filenamePlaceholders;

    Q_SIGNALS:

    void errorMessage(const QString &str);
    void pixmapChanged(const QPixmap &pixmap);
    void imageSaved(const QUrl &savedAt);
    void forceNotify(const QUrl &savedAt);

    public Q_SLOTS:

    QUrl getAutosaveFilename();
    QUrl tempSave(const QString &mimetype = QStringLiteral("png"));

    void doSave(const QUrl &url = QUrl(), bool notify = false);
    bool doSaveAs(QWidget *parentWindow = nullptr, bool notify = false);
    void doCopyToClipboard(bool notify);
    void doPrint(QPrinter *printer);

    private:

    QString truncatedFilename(const QString &filename);
    QString makeAutosaveFilename();
    using FileNameAlreadyUsedCheck = bool (ExportManager::*)(const QUrl&) const;
    QString autoIncrementFilename(const QString &baseName, const QString &extension,
                                  FileNameAlreadyUsedCheck isFileNameUsed);
    QString makeSaveMimetype(const QUrl &url);
    bool writeImage(QIODevice *device, const QByteArray &format);
    bool save(const QUrl &url);
    bool localSave(const QUrl &url, const QString &mimetype);
    bool remoteSave(const QUrl &url, const QString &mimetype);
    bool isTempFileAlreadyUsed(const QUrl &url) const;

    QPixmap mSavePixmap;
    QDateTime mPixmapTimestamp;
    QUrl mTempFile;
    QTemporaryDir *mTempDir;
    QList<QUrl> mUsedTempFileNames;
    QString mWindowTitle;
    ImageGrabber::GrabMode mGrabMode;
};

#endif // EXPORTMANAGER_H
