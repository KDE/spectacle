/* This file is part of Spectacle, the KDE screenshot utility
 * Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <SpectacleCommon.h>

#include <QObject>
#include <QIODevice>
#include <QMap>
#include <QPrinter>
#include <QPixmap>
#include <QDateTime>
#include <QUrl>
#include <KLocalizedString>

class QTemporaryDir;

class ExportManager: public QObject
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

    QString defaultSaveLocation() const;
    bool isFileExists(const QUrl &url) const;
    void setPixmap(const QPixmap &pixmap);
    QPixmap pixmap() const;
    void updatePixmapTimestamp();
    void setTimestamp(const QDateTime &timestamp);
    QString windowTitle() const;
    Spectacle::CaptureMode captureMode() const;
    void setCaptureMode(Spectacle::CaptureMode theCaptureMode);
    QString formatFilename(const QString &nameTemplate);

    static const QMap<QString, KLocalizedString> filenamePlaceholders;

    Q_SIGNALS:

    void errorMessage(const QString &str);
    void pixmapChanged(const QPixmap &pixmap);
    void imageSaved(const QUrl &savedAt);
    void imageCopied();
    void imageSavedAndCopied(const QUrl &savedAt);
    void forceNotify(const QUrl &savedAt);

    public Q_SLOTS:

    QUrl getAutosaveFilename();
    QUrl tempSave();

    void setWindowTitle(const QString &windowTitle);
    void doSave(const QUrl &url = QUrl(), bool notify = false);
    bool doSaveAs(QWidget *parentWindow = nullptr, bool notify = false);
    void doSaveAndCopy(const QUrl &url = QUrl());
    void doCopyToClipboard(bool notify = false);
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
    Spectacle::CaptureMode mCaptureMode { Spectacle::CaptureMode::AllScreens };
};
