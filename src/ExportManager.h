/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "settings.h"
#include <KLocalizedString>
#include <QDateTime>
class QIODevice;
#include <QMap>
#include <QObject>
class QPrinter;
#include <QUrl>

class QTemporaryDir;

class ExportManager : public QObject
{
    Q_OBJECT

    // singleton-ize the class

public:
    static ExportManager *instance();

private:
    explicit ExportManager(QObject *parent = nullptr);
    ~ExportManager() override;

    ExportManager(ExportManager const &) = delete;
    void operator=(ExportManager const &) = delete;

    // now the usual stuff

public:
    enum Action {
        NoActions   = 0b00000,
        Save        = 0b00001,
        SaveAs      = 0b00010,
        CopyImage   = 0b00100,
        CopyPath    = 0b01000,
        UserAction  = 0b10000,
        AnySave     = Save | SaveAs,
        AnyAction   = AnySave | CopyImage | CopyPath,
    };
    Q_DECLARE_FLAGS(Actions, Action)
    Q_FLAG(Action)

    QString defaultSaveLocation() const;
    QString defaultVideoSaveLocation() const;
    bool isFileExists(const QUrl &url) const;
    bool isImageSavedNotInTemp() const;
    void setImage(const QImage &image);
    QImage image() const;
    void updateTimestamp();
    void setTimestamp(const QDateTime &timestamp);

    /**
     * The title used to fill the window title template in formatted file names.
     */
    QString windowTitle() const;

    /**
     * Returns a formatted filename using a template string.
     */
    QString formattedFilename(const QString &nameTemplate = Settings::imageFilenameTemplate()) const;

    /**
     * The URL to record a video with before it is exported.
     */
    QUrl tempVideoUrl();

    const QTemporaryDir *temporaryDir();

    static const QMap<QString, KLocalizedString> filenamePlaceholders;

Q_SIGNALS:
    void imageChanged();

    void errorMessage(const QString &str);
    void imageExported(const ExportManager::Actions &actions, const QUrl &url = {});
    void videoExported(const ExportManager::Actions &actions, const QUrl &url = {});

public Q_SLOTS:

    QUrl getAutosaveFilename() const;
    QUrl tempSave();

    void setWindowTitle(const QString &windowTitle);
    void exportImage(ExportManager::Actions actions, QUrl url = {});
    void exportVideo(ExportManager::Actions actions, const QUrl &inputUrl, QUrl outputUrl = {});
    void doPrint(QPrinter *printer);

private:
    QString truncatedFilename(const QString &filename) const;
    using FileNameAlreadyUsedCheck = bool (ExportManager::*)(const QUrl &) const;
    QString autoIncrementFilename(const QString &baseName, const QString &extension, FileNameAlreadyUsedCheck isFileNameUsed) const;
    QString imageFileSuffix(const QUrl &url) const;
    bool writeImage(QIODevice *device, const QByteArray &suffix);
    bool save(const QUrl &url);
    bool localSave(const QUrl &url, const QString &suffix);
    bool remoteSave(const QUrl &url, const QString &suffix);
    bool isTempFileAlreadyUsed(const QUrl &url) const;

    bool m_imageSavedNotInTemp;
    QImage m_saveImage;
    QDateTime m_timestamp;
    QUrl m_tempFile;
    std::unique_ptr<QTemporaryDir> m_tempDir;
    QList<QUrl> m_usedTempFileNames;
    QString m_windowTitle;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ExportManager::Actions)
