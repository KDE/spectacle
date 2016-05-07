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
#include <QPrinter>
#include <QPixmap>
#include <QUrl>

class ExportManager : public QObject
{
    Q_OBJECT

    // singleton-ize the class

    public:

    static ExportManager* instance();

    private:

    explicit ExportManager(QObject *parent = 0);
    virtual ~ExportManager();

    ExportManager(ExportManager const&) = delete;
    void operator= (ExportManager const&) = delete;

    // now the usual stuff

    public:

    Q_PROPERTY(QString saveLocation READ saveLocation WRITE setSaveLocation NOTIFY saveLocationChanged)
    Q_PROPERTY(QPixmap pixmap READ pixmap WRITE setPixmap NOTIFY pixmapChanged)

    void setSaveLocation(const QString &location);
    QString saveLocation() const;
    void setPixmap(const QPixmap &pixmap);
    QPixmap pixmap() const;
    QString pixmapDataUri() const;

    signals:

    void errorMessage(const QString &str);
    void saveLocationChanged(const QString &location);
    void pixmapChanged(const QPixmap &pixmap);
    void imageSaved(const QUrl &savedAt);
    void forceNotify(const QUrl &savedAt);

    public slots:

    QUrl getAutosaveFilename();
    QUrl tempSave(const QString &mimetype = "png");

    void doSave(const QUrl &url = QUrl(), bool notify = false);
    void doSaveAs(QWidget *parentWindow = 0);
    void doCopyToClipboard();
    void doPrint(QPrinter *printer);

    private:

    QString makeAutosaveFilename();
    QString autoIncrementFilename(const QString &baseName, const QString &extension);
    QString makeSaveMimetype(const QUrl &url);
    bool writeImage(QIODevice *device, const QByteArray &format);
    bool save(const QUrl &url);
    bool localSave(const QUrl &url, const QString &mimetype);
    bool remoteSave(const QUrl &url, const QString &mimetype);
    bool isFileExists(const QUrl &url);

    QPixmap mSavePixmap;
    QUrl mTempFile;
};

#endif // EXPORTMANAGER_H
