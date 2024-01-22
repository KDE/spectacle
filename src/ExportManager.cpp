/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ExportManager.h"

#include "settings.h"
#include <kio_version.h>

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileDialog>
#include <QImageWriter>
#include <QLocale>
#include <QMimeData>
#include <QMimeDatabase>
#include <QPainter>
#include <QPrinter>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QString>
#include <QTemporaryDir>
#include <QTemporaryFile>

#include <KIO/DeleteJob>
#include <KIO/FileCopyJob>
#include <KIO/ListJob>
#include <KIO/MkpathJob>
#include <KIO/StatJob>
#include <KRecentDocument>
#include <KSharedConfig>
#include <KSystemClipboard>
#include <ZXing/ReadBarcode.h>

using namespace Qt::StringLiterals;

ExportManager::ExportManager(QObject *parent)
    : QObject(parent)
    , m_imageSavedNotInTemp(false)
    , m_saveImage(QImage())
    , m_tempFile(QUrl())
{
    connect(this, &ExportManager::imageExported, [](Actions actions, const QUrl &url) {
        if (actions & AnySave) {
            Settings::setLastImageSaveLocation(url);
            if (actions & SaveAs) {
                Settings::setLastImageSaveAsLocation(url);
            }
        }
    });
}

ExportManager::~ExportManager() = default;

ExportManager *ExportManager::instance()
{
    static ExportManager instance;
    // Ensure the SystemClipboard is instantiated early enough
    KSystemClipboard::instance();
    return &instance;
}

// screenshot image setter and getter

QImage ExportManager::image() const
{
    return m_saveImage;
}

void ExportManager::setWindowTitle(const QString &windowTitle)
{
    m_windowTitle = windowTitle;
}

QString ExportManager::windowTitle() const
{
    return m_windowTitle;
}

void ExportManager::setImage(const QImage &image)
{
    m_saveImage = image;

    // reset our saved tempfile
    if (m_tempFile.isValid()) {
        m_usedTempFileNames.append(m_tempFile);
        QFile file(m_tempFile.toLocalFile());
        file.remove();
        m_tempFile = QUrl();
    }

    // since the image was modified, we now consider the image unsaved
    m_imageSavedNotInTemp = false;

    Q_EMIT imageChanged();
}

void ExportManager::updateTimestamp()
{
    m_timestamp = QDateTime::currentDateTime();
}

void ExportManager::setTimestamp(const QDateTime &timestamp)
{
    m_timestamp = timestamp;
}

// native file save helpers

static QString ensureDefaultLocationExists(const QUrl &saveUrl)
{
    QString savePath = saveUrl.isRelative() ? saveUrl.toString() : saveUrl.toLocalFile();
    savePath = QDir::cleanPath(savePath);

    QDir savePathDir(savePath);
    if (!(savePathDir.exists())) {
        savePathDir.mkpath(u"."_s);
    }
    return savePath;
}

QString ExportManager::defaultSaveLocation() const
{
    return ensureDefaultLocationExists(Settings::imageSaveLocation());
}

QString ExportManager::defaultVideoSaveLocation() const
{
    return ensureDefaultLocationExists(Settings::videoSaveLocation());
}

QUrl ExportManager::getAutosaveFilename() const
{
    const QString baseDir = defaultSaveLocation();
    const QDir baseDirPath(baseDir);
    const QString filename = formattedFilename();
    const QString fullpath = autoIncrementFilename(baseDirPath.filePath(filename),
                                                   Settings::preferredImageFormat().toLower(),
                                                   &ExportManager::isFileExists);

    const QUrl fileNameUrl = QUrl::fromUserInput(fullpath);
    if (fileNameUrl.isValid()) {
        return fileNameUrl;
    } else {
        return QUrl();
    }
}

QUrl ExportManager::tempVideoUrl()
{
    const auto format = static_cast<VideoPlatform::Format>(Settings::preferredVideoFormat());
    auto extension = VideoPlatform::extensionForFormat(format);
    QString baseDir = defaultVideoSaveLocation();
    const QDir baseDirPath(baseDir);
    const QString filename = formattedFilename(Settings::videoFilenameTemplate());
    QString filepath = autoIncrementFilename(baseDirPath.filePath(filename), extension, &ExportManager::isFileExists);

    auto tempDir = temporaryDir();
    if (!tempDir) {
        return {};
    }

    if (!baseDir.isEmpty() && !baseDir.endsWith(u'/')) {
        baseDir += u'/';
    }
    filepath = tempDir->path() + u'/' + filepath.right(filepath.size() - baseDir.size());
    return QUrl::fromLocalFile(filepath);
}

const QTemporaryDir *ExportManager::temporaryDir()
{
    if (!m_tempDir) {
        // Cleanup Spectacle's temp dirs after startup instead of while quitting.
        const auto filters = QDir::Filter::Dirs | QDir::NoDotAndDotDot | QDir::CaseSensitive | QDir::NoSymLinks;
        const auto oldDirs = QDir::temp().entryList({u"Spectacle.??????"_s}, filters);
        QList<QUrl> oldUrls;
        for (const auto &dir : oldDirs) {
            oldUrls << QUrl::fromLocalFile(QDir::tempPath() + u'/' + dir);
        }
        KIO::del(oldUrls, KIO::HideProgressInfo)->start();
        m_tempDir = std::make_unique<QTemporaryDir>(QDir::tempPath() + u"/Spectacle.XXXXXX"_s);
        m_tempDir->setAutoRemove(false);
    }
    return m_tempDir->isValid() ? m_tempDir.get() : nullptr;
}

QString ExportManager::truncatedFilename(QString const &filename) const
{
    QString result = filename;
    constexpr auto maxFilenameLength = 255;
    constexpr auto maxExtensionLength = 5; // For example, ".jpeg"
    constexpr auto maxCounterLength = 20; // std::numeric_limits<quint64>::max() == 18446744073709551615
    constexpr auto maxLength = maxFilenameLength - maxCounterLength - maxExtensionLength;
    result.truncate(maxLength);
    return result;
}

QString ExportManager::formattedFilename(const QString &nameTemplate) const
{
    const QDateTime timestamp = m_timestamp;
    QString baseName = nameTemplate;
    QString baseDir = defaultSaveLocation();
    QString title = m_windowTitle;

    if (!title.isEmpty()) {
        title.replace(u'/', u'_'); // POSIX doesn't allow "/" in filenames
    } else {
        // Remove <title> with separators around it
        const auto wordSymbol = uR"(\p{L}\p{M}\p{N})"_s;
        const auto separator = u"([^%1]+)"_s.arg(wordSymbol);
        const auto re = QRegularExpression(u"(.*?)(%1<title>|<title>%1)(.*?)"_s.arg(separator));
        baseName.replace(re, uR"(\1\5)"_s);
    }

    QString result = baseName;
    const auto &locale = QLocale::system();
    // QDateTime
    for (auto it = filenamePlaceholders.cbegin(); it != filenamePlaceholders.cend(); ++it) {
        if (it->flags.testFlags(Placeholder::QDateTime)) {
            result.replace(it->plainKey, locale.toString(timestamp, it->baseKey));
        }
    }
    // Manual interpretation
    // `h` and `hh` in QDateTime::toString or QLocale::toString
    // are only 12 hour when paired with an AM/PM display in the same toString call,
    // so we have to get the 12 hour format for `<h>` and `<hh>` manually.
    result.replace("<h>"_L1, timestamp.toString(u"hAP"_s).chopped(2));
    result.replace("<hh>"_L1, timestamp.toString(u"hhAP"_s).chopped(2));
    result.replace("<UnixTime>"_L1, QString::number(timestamp.toSecsSinceEpoch()));
    result.replace("<title>"_L1, title);

    // check if basename includes %[N]d token for sequential file numbering
    QRegularExpression paddingRE;
    paddingRE.setPattern(u"<(#+)>"_s);
    QRegularExpressionMatchIterator it = paddingRE.globalMatch(result);
    if (it.hasNext()) {
        // strip any subdirectories from the template to construct the filename matching regex
        // we are matching filenames only, not paths
        QString resultCopy = QRegularExpression::escape(result.section(u'/', -1));
        QList<QRegularExpressionMatch> matches;
        while (it.hasNext()) {
            QRegularExpressionMatch paddingMatch = it.next();
            matches.push_back(paddingMatch);
            // determine padding value
            int paddedLength = 1;
            if (!paddingMatch.captured(1).isEmpty()) {
                paddedLength = paddingMatch.captured(1).length();
            }
            QString escapedMatch = QRegularExpression::escape(paddingMatch.captured());
            resultCopy.replace(escapedMatch, u"(\\d{%1,})"_s.arg(QString::number(paddedLength)));
        }
        if (result.contains(u'/')) {
            // In case the filename template contains a subdirectory,
            // we need to search for files in the subdirectory instead of the baseDir.
            // so let's add that to baseDir before we search for files.
            baseDir += u"/%1"_s.arg(result.section(u'/', 0, -2));
        }
        // search save directory for files
        QDir dir(baseDir);
        const QStringList fileNames = dir.entryList(QDir::Files, QDir::Name);
        int highestFileNumber = 0;

        // if there are files in the directory...
        if (fileNames.length() > 0) {
            QRegularExpression fileNumberRE;
            fileNumberRE.setPattern(resultCopy);
            // ... check the file names for string matching token with padding specified in result
            const QStringList filteredFiles = fileNames.filter(fileNumberRE);
            // if there are files in the directory that look like the file name with sequential numbering
            if (filteredFiles.length() > 0) {
                // loop through filtered file names looking for highest number
                for (const QString &filteredFile : filteredFiles) {
                    int currentFileNumber = fileNumberRE.match(filteredFile).captured(1).length();
                    if (currentFileNumber > highestFileNumber) {
                        highestFileNumber = currentFileNumber;
                    }
                }
            }
        }
        // replace placeholder with next number padded
        for (const auto &match : matches) {
            int paddedLength = 1;
            if (!match.captured(1).isEmpty()) {
                paddedLength = match.captured(1).length();
            }
            const QString nextFileNumberPadded = QString::number(highestFileNumber + 1).rightJustified(paddedLength, u'0');
            result.replace(match.captured(), nextFileNumberPadded);
        }
    }

    // Remove leading and trailing '/'
    while (result.startsWith(u'/')) {
        result.remove(0, 1);
    }
    while (result.endsWith(u'/')) {
        result.chop(1);
    }

    if (result.isEmpty()) {
        result = u"Screenshot"_s;
    }
    return truncatedFilename(result);
}

QString ExportManager::autoIncrementFilename(const QString &baseName, const QString &extension, FileNameAlreadyUsedCheck isFileNameUsed) const
{
    QString result = truncatedFilename(baseName) + u'.' + extension;
    if (!((this->*isFileNameUsed)(QUrl::fromUserInput(result)))) {
        return result;
    }

    QString fileNameFmt = truncatedFilename(baseName) + u"-%1.";
    for (quint64 i = 1; i < std::numeric_limits<quint64>::max(); i++) {
        result = fileNameFmt.arg(i) + extension;
        if (!((this->*isFileNameUsed)(QUrl::fromUserInput(result)))) {
            return result;
        }
    }

    // unlikely this will ever happen, but just in case we've run
    // out of numbers

    result = fileNameFmt.arg(u"OVERFLOW-" + QString::number(QRandomGenerator::global()->bounded(10000)));
    return truncatedFilename(result) + extension;
}

QString ExportManager::imageFileSuffix(const QUrl &url) const
{
    QMimeDatabase mimedb;
    const QString type = mimedb.mimeTypeForUrl(url).preferredSuffix();

    if (type.isEmpty()) {
        return Settings::self()->preferredImageFormat().toLower();
    }
    return type;
}

bool ExportManager::writeImage(QIODevice *device, const QByteArray &suffix)
{
    // In the documentation for QImageWriter, it is a bit ambiguous what "format" means.
    // From looking at how QImageWriter handles the built-in supported formats internally,
    // "format" basically means the file extension, not the mimetype.
    QImageWriter imageWriter(device, suffix);
    imageWriter.setQuality(Settings::imageCompressionQuality());
    /** Set compression 50 if the format is png. Otherwise if no compression value is specified
     *  it will fallback to using quality (QTBUG-43618) and produce huge files.
     *  See also qpnghandler.cpp#n1075. The other formats that do compression seem to have it
     *  enabled by default and only disabled if compression is set to 0, also any value except 0
     *  has the same effect for them.
     */
    if (suffix == "png") {
        imageWriter.setCompression(50);
    }
    if (!(imageWriter.canWrite())) {
        Q_EMIT errorMessage(i18n("QImageWriter cannot write image: %1", imageWriter.errorString()));
        return false;
    }

    return imageWriter.write(m_saveImage);
}

bool ExportManager::localSave(const QUrl &url, const QString &suffix)
{
    // Create save directory if it doesn't exist
    const QUrl dirPath(url.adjusted(QUrl::RemoveFilename));
    const QDir dir(dirPath.path());

    if (!dir.mkpath(u"."_s)) {
        Q_EMIT errorMessage(xi18nc("@info",
                                   "Cannot save screenshot because creating "
                                   "the directory failed:<nl/><filename>%1</filename>",
                                   dirPath.path()));
        return false;
    }

    QFile outputFile(url.toLocalFile());

    outputFile.open(QFile::WriteOnly);
    if (!writeImage(&outputFile, suffix.toLatin1())) {
        Q_EMIT errorMessage(i18n("Cannot save screenshot. Error while writing file."));
        return false;
    }
    return true;
}

bool ExportManager::remoteSave(const QUrl &url, const QString &suffix)
{
    // Check if remote save directory exists
    const QUrl dirPath(url.adjusted(QUrl::RemoveFilename));
    KIO::ListJob *listJob = KIO::listDir(dirPath);
    listJob->exec();

    if (listJob->error() != KJob::NoError) {
        // Create remote save directory
        KIO::MkpathJob *mkpathJob = KIO::mkpath(dirPath, QUrl(defaultSaveLocation()));
        mkpathJob->exec();

        if (mkpathJob->error() != KJob::NoError) {
            Q_EMIT errorMessage(xi18nc("@info",
                                       "Cannot save screenshot because creating the "
                                       "remote directory failed:<nl/><filename>%1</filename>",
                                       dirPath.path()));
            return false;
        }
    }

    QTemporaryFile tmpFile;

    if (tmpFile.open()) {
        if (!writeImage(&tmpFile, suffix.toLatin1())) {
            Q_EMIT errorMessage(i18n("Cannot save screenshot. Error while writing temporary local file."));
            return false;
        }

        KIO::FileCopyJob *uploadJob = KIO::file_copy(QUrl::fromLocalFile(tmpFile.fileName()), url);
        uploadJob->exec();

        if (uploadJob->error() != KJob::NoError) {
            Q_EMIT errorMessage(i18n("Unable to save image. Could not upload file to remote location."));
            return false;
        }
        return true;
    }

    return false;
}

QUrl ExportManager::tempSave()
{
    // if we already have a temp file saved, use that
    if (m_tempFile.isValid()) {
        if (QFile(m_tempFile.toLocalFile()).exists()) {
            return m_tempFile;
        }
    }

    auto tempDir = this->temporaryDir();
    if (tempDir) {
        // create the temporary file itself with normal file name and also unique one for this session
        // supports the use-case of creating multiple screenshots in a row
        // and exporting them to the same destination e.g. via clipboard,
        // where the temp file name is used as filename suggestion
        const QString baseFileName = m_tempDir->path() + u'/' + QUrl::fromLocalFile(formattedFilename()).fileName();

        QString suffix = imageFileSuffix(QUrl(baseFileName));
        const QString fileName = autoIncrementFilename(baseFileName, suffix, &ExportManager::isTempFileAlreadyUsed);
        QFile tmpFile(fileName);
        if (tmpFile.open(QFile::WriteOnly)) {
            if (writeImage(&tmpFile, suffix.toLatin1())) {
                m_tempFile = QUrl::fromLocalFile(tmpFile.fileName());
                // try to make sure 3rd-party which gets the url of the temporary file e.g. on export
                // properly treats this as readonly, also hide from other users
                tmpFile.setPermissions(QFile::ReadUser);
                return m_tempFile;
            }
        }
    }

    Q_EMIT errorMessage(i18n("Cannot save screenshot. Error while writing temporary local file."));
    return QUrl();
}

bool ExportManager::save(const QUrl &url)
{
    if (!(url.isValid())) {
        Q_EMIT errorMessage(i18n("Cannot save screenshot. The save filename is invalid."));
        return false;
    }

    const QString suffix = imageFileSuffix(url);
    bool saveSucceded = false;
    if (url.isLocalFile()) {
        saveSucceded = localSave(url, suffix);
    } else {
        saveSucceded = remoteSave(url, suffix);
    }
    if (saveSucceded) {
        m_imageSavedNotInTemp = true;
        KRecentDocument::add(url, QGuiApplication::desktopFileName());
    }
    return saveSucceded;
}

bool ExportManager::isFileExists(const QUrl &url) const
{
    if (!(url.isValid())) {
        return false;
    }
    // Using StatJob instead of QFileInfo::exists() is necessary for checking non-local URLs.
    KIO::StatJob *existsJob = KIO::stat(url, KIO::StatJob::DestinationSide, KIO::StatNoDetails, KIO::HideProgressInfo);

    existsJob->exec();

    return (existsJob->error() == KJob::NoError);
}

bool ExportManager::isImageSavedNotInTemp() const
{
    return m_imageSavedNotInTemp;
}

bool ExportManager::isTempFileAlreadyUsed(const QUrl &url) const
{
    return m_usedTempFileNames.contains(url);
}

void ExportManager::exportImage(ExportManager::Actions actions, QUrl url)
{
    if (m_saveImage.isNull() && actions & (Save | SaveAs | CopyImage)) {
        Q_EMIT errorMessage(i18n("Cannot save an empty screenshot image."));
        return;
    }

    bool success = false;
    if (actions & SaveAs) {
        QStringList supportedFilters;

        // construct the supported mimetype list
        const auto mimeTypes = QImageWriter::supportedMimeTypes();
        supportedFilters.reserve(mimeTypes.count());
        for (const auto &mimeType : mimeTypes) {
            supportedFilters.append(QString::fromUtf8(mimeType).trimmed());
        }

        // construct the file name
        const QString filenameExtension = Settings::self()->preferredImageFormat().toLower();
        const QString mimetype = QMimeDatabase().mimeTypeForFile(u"~/fakefile."_s + filenameExtension, QMimeDatabase::MatchExtension).name();
        QFileDialog dialog;
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.setFileMode(QFileDialog::AnyFile);
        QUrl dirUrl = url.adjusted(QUrl::RemoveFilename);
        if (!dirUrl.isValid()) {
            dirUrl = Settings::self()->lastImageSaveAsLocation().adjusted(QUrl::RemoveFilename);
        }
        dialog.setDirectoryUrl(dirUrl);
        dialog.selectFile(formattedFilename() + u"."_s + filenameExtension);
        dialog.setDefaultSuffix(u"."_s + filenameExtension);
        dialog.setMimeTypeFilters(supportedFilters);
        dialog.selectMimeTypeFilter(mimetype);

        // launch the dialog
        const bool accepted = dialog.exec() == QDialog::Accepted;
        if (accepted && !dialog.selectedUrls().isEmpty()) {
            url = dialog.selectedUrls().constFirst();
        }
        actions.setFlag(SaveAs, accepted && url.isValid());
    }

    bool saved = actions & AnySave;
    if (saved) {
        if (!url.isValid()) {
            url = getAutosaveFilename();
        }
        saved = success = save(url);
        if (!success) {
            actions.setFlag(Save, false);
            actions.setFlag(SaveAs, false);
        }
    }

    if (actions & CopyImage) {
        auto data = new QMimeData();
        bool savedLocal = saved && url.isLocalFile();
        bool canWriteTemp = temporaryDir() != nullptr;
        if (savedLocal || canWriteTemp) {
            QString localFilePath;
            if (savedLocal || (url.isEmpty() && canWriteTemp)) {
                if (url.isEmpty()) {
                    url = tempSave();
                }
                localFilePath = url.toLocalFile();
            } else {
                localFilePath = tempSave().toLocalFile();
            }
            QFile imageFile(localFilePath);
            imageFile.open(QFile::ReadOnly);
            const auto mimetype = QMimeDatabase().mimeTypeForFile(localFilePath);
            data->setData(mimetype.name(), imageFile.readAll());
            imageFile.close();
        } else {
            // Fallback to the old way if we can't save a temp file for some reason.
            data->setImageData(m_saveImage);
        }
        // "x-kde-force-image-copy" is handled by Klipper.
        // It ensures that the image is copied to Klipper even with the
        // "Non-text selection: Never save in history" setting selected in Klipper.
        data->setData(u"x-kde-force-image-copy"_s, QByteArray());
        KSystemClipboard::instance()->setMimeData(data, QClipboard::Clipboard);
        success = true;
    }

    if (actions & CopyPath
        // This behavior has no relation to the setting in the config UI,
        // but it was added to solve this feature request:
        // https://bugs.kde.org/show_bug.cgi?id=357423
        || (saved && Settings::clipboardGroup() == Settings::PostScreenshotCopyLocation)) {
        if (!url.isValid()) {
            if (m_imageSavedNotInTemp) {
                // The image has been saved (manually or automatically),
                // we need to choose that file path
                url = Settings::self()->lastImageSaveLocation();
            } else {
                // use a temporary save path, and copy that to clipboard instead
                url = ExportManager::instance()->tempSave();
            }
        }

        // will be deleted for us by the platform's clipboard manager.
        auto data = new QMimeData();
        data->setText(url.toLocalFile());
        KSystemClipboard::instance()->setMimeData(data, QClipboard::Clipboard);
        success = true;
    }

    if (success) {
        Q_EMIT imageExported(actions, url);
    }
}

void ExportManager::scanQRCode()
{
    auto image = ExportManager::instance()->image();
    image.convertTo(QImage::Format_Mono);
    image.convertTo(QImage::Format_Grayscale8);

    auto zximage = ZXing::ImageView(image.constBits(), image.width(), image.height(), ZXing::ImageFormat::Lum);
    auto results = ZXing::ReadBarcodes(zximage, {});

    if (!results.empty()) {
        QString result = QString::fromStdString(results[0].text());

        auto data = new QMimeData();
        data->setText(result);
        KSystemClipboard::instance()->setMimeData(data, QClipboard::Clipboard);
        Q_EMIT qrCodeScanned(result);
    }
}

void ExportManager::exportVideo(ExportManager::Actions actions, const QUrl &inputUrl, QUrl outputUrl)
{
    // input can be empty or nonexistent, but not if we're saving
    const auto &inputFile = inputUrl.toLocalFile();
    const auto &inputName = inputUrl.fileName();
    if ((inputName.isEmpty() || !QFileInfo::exists(inputFile)) && actions & (Save | SaveAs)) {
        Q_EMIT errorMessage(i18nc("@info:shell","Failed to export video: Temporary file URL must be an existing local file"));
        return;
    }

    // output can be empty, but not invalid or with an empty name when not empty and saving
    const auto &outputName = outputUrl.fileName();
    if (!outputUrl.isEmpty() && (!outputUrl.isValid() || outputName.isEmpty()) && actions & (Save | SaveAs)) {
        Q_EMIT errorMessage(i18nc("@info:shell","Failed to export video: Output file URL must be a valid URL with a file name"));
        return;
    }

    if (actions & SaveAs) {
        QMimeDatabase mimeDatabase;
        // construct the file name
        const auto &extension = inputName.mid(inputName.lastIndexOf(u'.'));
        const auto &mimetype = mimeDatabase.mimeTypeForFile(inputName, QMimeDatabase::MatchExtension).name();
        QFileDialog dialog;
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.setFileMode(QFileDialog::AnyFile);
        const auto outputDir = outputUrl.adjusted(QUrl::RemoveFilename);
        if (!outputDir.isValid()) {
            dialog.setDirectoryUrl(Settings::self()->lastVideoSaveAsLocation().adjusted(QUrl::RemoveFilename));
        } else {
            dialog.setDirectoryUrl(outputUrl);
        }
        dialog.setDefaultSuffix(extension);
        dialog.selectFile(!outputName.isEmpty() ? outputName : inputName);
        dialog.setMimeTypeFilters({mimetype});
        dialog.selectMimeTypeFilter(mimetype);

        // launch the dialog
        const bool accepted = dialog.exec() == QDialog::Accepted;
        const auto &selectedUrl = dialog.selectedUrls().value(0, QUrl());
        if (accepted && !selectedUrl.fileName().isEmpty()) {
            outputUrl = selectedUrl;
        }
        actions.setFlag(SaveAs, accepted && outputUrl.isValid());
    }

    bool inputFromTemp = temporaryDir() && inputFile.startsWith(m_tempDir->path());
    if (!outputUrl.isValid()) {
        if (actions & AnySave && inputFromTemp) {
            // Use the temp url without the temp dir as the new url, if necessary
            const auto &tempDirPath = m_tempDir->path() + u'/';
            const auto &reducedPath = inputFile.right(inputFile.size() - tempDirPath.size());
            outputUrl = Settings::videoSaveLocation().adjusted(QUrl::StripTrailingSlash);
            outputUrl.setPath(outputUrl.path() + u'/' + reducedPath);
        } else {
            outputUrl = inputUrl;
        }
    }

    // When the input is the output, it should still count as a successful save
    bool saved = inputUrl == outputUrl;
    if (actions & AnySave && !saved) {
        const auto &saveDirUrl = outputUrl.adjusted(QUrl::RemoveFilename);
        bool saveDirExists = false;
        if (saveDirUrl.isLocalFile()) {
            saveDirExists = QFileInfo::exists(saveDirUrl.toLocalFile());
        } else {
            KIO::ListJob *listJob = KIO::listDir(saveDirUrl);
            listJob->exec();
            saveDirExists = listJob->error() == KJob::NoError;
        }
        if (!saveDirExists) {
            KIO::MkpathJob *mkpathJob = KIO::mkpath(saveDirUrl);
            mkpathJob->exec();
            saveDirExists = mkpathJob->error() == KJob::NoError;
        }
        if (saveDirExists) {
            if (inputFromTemp) {
                auto fileMoveJob = KIO::file_move(inputUrl, outputUrl);
                fileMoveJob->exec();
                saved = fileMoveJob->error() == KJob::NoError;
            } else {
                auto fileCopyJob = KIO::file_copy(inputUrl, outputUrl);
                fileCopyJob->exec();
                saved = fileCopyJob->error() == KJob::NoError;
            }
        }
        if (!saved) {
            actions.setFlag(AnySave, false);
            Q_EMIT errorMessage(i18nc("@info", "Unable to save recording. Could not move file to location: %1", outputUrl.toString()));
            return;
        }
    }

    bool copiedPath = false;
    if (actions & CopyPath
        // This behavior has no relation to the setting in the config UI,
        // but it was added to solve this feature request:
        // https://bugs.kde.org/show_bug.cgi?id=357423
        || (saved && Settings::clipboardGroup() == Settings::PostScreenshotCopyLocation)) {
        // will be deleted for us by the platform's clipboard manager.
        auto data = new QMimeData();
        data->setText(outputUrl.isLocalFile() ? outputUrl.toLocalFile() : outputUrl.toString());
        KSystemClipboard::instance()->setMimeData(data, QClipboard::Clipboard);
        copiedPath = true;
    }

    if (saved || copiedPath) {
        Q_EMIT videoExported(actions, outputUrl);
    }
}

void ExportManager::doPrint(QPrinter *printer)
{
    QPainter painter;

    if (!(painter.begin(printer))) {
        Q_EMIT errorMessage(i18n("Printing failed. The printer failed to initialize."));
        return;
    }

    painter.setRenderHint(QPainter::LosslessImageRendering);

    QRect devRect(0, 0, printer->width(), printer->height());
    QImage image = m_saveImage.scaled(devRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QRect srcRect = image.rect();
    srcRect.moveCenter(devRect.center());

    painter.drawImage(srcRect.topLeft(), image);
    painter.end();
}

KLocalizedString placeholderDescription(const char *string)
{
    return ki18nc("A placeholder in the user configurable filename will replaced by the specified value", string);
}

using enum ExportManager::Placeholder::Flag;
const QList<ExportManager::Placeholder> ExportManager::filenamePlaceholders{
    {Date | Extra | QDateTime, u"d"_s, placeholderDescription("Day (1–31)")},
    {Date | QDateTime, u"dd"_s, placeholderDescription("Day (01–31)")},
    {Date | Extra | QDateTime, u"ddd"_s, placeholderDescription("Day (short name)")},
    {Date | Extra | QDateTime, u"dddd"_s, placeholderDescription("Day (long name)")},
    {Date | Extra | QDateTime, u"M"_s, placeholderDescription("Month (1–12)")},
    {Date | QDateTime, u"MM"_s, placeholderDescription("Month (01–12)")},
    {Date | QDateTime, u"MMM"_s, placeholderDescription("Month (short name)")},
    {Date | QDateTime, u"MMMM"_s, placeholderDescription("Month (long name)")},
    {Date | QDateTime, u"yy"_s, placeholderDescription("Year (2 digit)")},
    {Date | QDateTime, u"yyyy"_s, placeholderDescription("Year (4 digit)")},
    {Time | Extra | QDateTime, u"H"_s, placeholderDescription("Hour (0–23)")},
    {Time | QDateTime, u"HH"_s, placeholderDescription("Hour (00–23)")},
    {Time | Extra, u"h"_s, placeholderDescription("Hour (1–12)")},
    {Time, u"hh"_s, placeholderDescription("Hour (01–12)")},
    {Time | Extra | QDateTime, u"m"_s, placeholderDescription("Minute (0–59)")},
    {Time | QDateTime, u"mm"_s, placeholderDescription("Minute (00–59)")},
    {Time | Extra | QDateTime, u"s"_s, placeholderDescription("Second (0–59)")},
    {Time | QDateTime, u"ss"_s, placeholderDescription("Second (00–59)")},
    {Time | Extra | QDateTime, u"z"_s, placeholderDescription("Millisecond (0–999)")},
    {Time | Extra | QDateTime, u"zz"_s}, // same as `z`
    {Time | Extra | QDateTime, u"zzz"_s, placeholderDescription("Millisecond (000–999)")},
    {Time | Extra | QDateTime, u"AP"_s, placeholderDescription("AM/PM")},
    {Time | Extra | QDateTime, u"A"_s}, // same as `AP`
    {Time | Extra | QDateTime, u"ap"_s, placeholderDescription("am/pm")},
    {Time | Extra | QDateTime, u"a"_s}, // same as `ap`
    {Time | QDateTime, u"Ap"_s, placeholderDescription("AM/PM or am/pm (localized)")},
    {Time | Extra | QDateTime, u"aP"_s}, // same as `Ap`
    {Time | Extra | QDateTime, u"t"_s, placeholderDescription("Timezone (short name)")},
    {Time | QDateTime, u"tt"_s, placeholderDescription("Timezone offset (±0000)")},
    {Time | Extra | QDateTime, u"ttt"_s, placeholderDescription("Timezone offset (±00:00)")},
    {Time | Extra | QDateTime, u"tttt"_s, placeholderDescription("Timezone (long name)")},
    {Time | Extra, u"UnixTime"_s, placeholderDescription("Seconds since the Unix epoch")},
    {Other, u"title"_s, placeholderDescription("Window Title")},
    {Other, u"#"_s, placeholderDescription("Sequential numbering, padded by inserting additional '#' characters")},
};

#include "moc_ExportManager.cpp"
