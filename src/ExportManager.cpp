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

#include <KIO/FileCopyJob>
#include <KIO/ListJob>
#include <KIO/MkpathJob>
#include <KIO/StatJob>
#include <KRecentDocument>
#include <KSharedConfig>
#include <KSystemClipboard>

ExportManager::ExportManager(QObject *parent)
    : QObject(parent)
    , m_imageSavedNotInTemp(false)
    , m_saveImage(QImage())
    , m_tempFile(QUrl())
{
    connect(this, &ExportManager::imageExported, [](Actions actions, const QUrl &url) {
        if (actions & AnySave) {
            Settings::setLastSaveLocation(url);
            if (actions & SaveAs) {
                Settings::setLastSaveAsLocation(url);
            }
        }
    });
}

ExportManager::~ExportManager()
{
    delete m_tempDir;
}

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
        savePathDir.mkpath(QStringLiteral("."));
    }
    return savePath;
}

QString ExportManager::defaultSaveLocation() const
{
    return ensureDefaultLocationExists(Settings::self()->defaultSaveLocation());
}

QString ExportManager::defaultVideoSaveLocation() const
{
    return ensureDefaultLocationExists(Settings::self()->defaultVideoSaveLocation());
}

QUrl ExportManager::getAutosaveFilename() const
{
    const QString baseDir = defaultSaveLocation();
    const QDir baseDirPath(baseDir);
    const QString filename = formattedFilename();
    const QString fullpath =
        autoIncrementFilename(baseDirPath.filePath(filename), Settings::self()->defaultSaveImageFormat().toLower(), &ExportManager::isFileExists);

    const QUrl fileNameUrl = QUrl::fromUserInput(fullpath);
    if (fileNameUrl.isValid()) {
        return fileNameUrl;
    } else {
        return QUrl();
    }
}

QString ExportManager::suggestedVideoFilename(const QString &extension) const
{
    const QString baseDir = defaultVideoSaveLocation();
    const QDir baseDirPath(baseDir);
    const QString filename = formattedFilename(Settings::self()->saveVideoFormat());
    const QString fullpath = autoIncrementFilename(baseDirPath.filePath(filename), extension, &ExportManager::isFileExists);

    const QUrl fileNameUrl = QUrl::fromUserInput(fullpath);
    if (fileNameUrl.isValid() && fileNameUrl.isLocalFile()) {
        return fileNameUrl.toLocalFile();
    } else {
        return {};
    }
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
        title.replace(QLatin1Char('/'), QLatin1String("_")); // POSIX doesn't allow "/" in filenames
    } else {
        // Remove '%T' with separators around it
        const auto wordSymbol = QStringLiteral(R"(\p{L}\p{M}\p{N})");
        const auto separator = QStringLiteral("([^%1]+)").arg(wordSymbol);
        const auto re = QRegularExpression(QStringLiteral("(.*?)(%1%T|%T%1)(.*?)").arg(separator));
        baseName.replace(re, QStringLiteral(R"(\1\5)"));
    }

    QString result = baseName.replace(QLatin1String("%Y"), timestamp.toString(QStringLiteral("yyyy")))
                             .replace(QLatin1String("%y"), timestamp.toString(QStringLiteral("yy")))
                             .replace(QLatin1String("%M"), timestamp.toString(QStringLiteral("MM")))
                             .replace(QLatin1String("%n"), timestamp.toString(QStringLiteral("MMM")))
                             .replace(QLatin1String("%N"), timestamp.toString(QStringLiteral("MMMM")))
                             .replace(QLatin1String("%D"), timestamp.toString(QStringLiteral("dd")))
                             .replace(QLatin1String("%H"), timestamp.toString(QStringLiteral("hh")))
                             .replace(QLatin1String("%m"), timestamp.toString(QStringLiteral("mm")))
                             .replace(QLatin1String("%S"), timestamp.toString(QStringLiteral("ss")))
                             .replace(QLatin1String("%t"), timestamp.toString(QStringLiteral("t")))
                             .replace(QLatin1String("%T"), title);

    // check if basename includes %[N]d token for sequential file numbering
    QRegularExpression paddingRE;
    paddingRE.setPattern(QStringLiteral("%(\\d*)d"));
    QRegularExpressionMatchIterator it = paddingRE.globalMatch(result);
    if (it.hasNext()) {
        // strip any subdirectories from the template to construct the filename matching regex
        // we are matching filenames only, not paths
        QString resultCopy = QRegularExpression::escape(result.section(QLatin1Char('/'), -1));
        QVector<QRegularExpressionMatch> matches;
        while (it.hasNext()) {
            QRegularExpressionMatch paddingMatch = it.next();
            matches.push_back(paddingMatch);
            // determine padding value
            int paddedLength = 1;
            if (!paddingMatch.captured(1).isEmpty()) {
                paddedLength = paddingMatch.captured(1).toInt();
            }
            QString escapedMatch = QRegularExpression::escape(paddingMatch.captured());
            resultCopy.replace(escapedMatch, QStringLiteral("(\\d{%1,})").arg(QString::number(paddedLength)));
        }
        if (result.contains(QLatin1Char('/'))) {
            // In case the filename template contains a subdirectory,
            // we need to search for files in the subdirectory instead of the baseDir.
            // so let's add that to baseDir before we search for files.
            baseDir += QStringLiteral("/%1").arg(result.section(QLatin1Char('/'), 0, -2));
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
                    int currentFileNumber = fileNumberRE.match(filteredFile).captured(1).toInt();
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
                paddedLength = match.captured(1).toInt();
            }
            const QString nextFileNumberPadded = QString::number(highestFileNumber + 1).rightJustified(paddedLength, QLatin1Char('0'));
            result.replace(match.captured(), nextFileNumberPadded);
        }
    }

    // Remove leading and trailing '/'
    while (result.startsWith(QLatin1Char('/'))) {
        result.remove(0, 1);
    }
    while (result.endsWith(QLatin1Char('/'))) {
        result.chop(1);
    }

    if (result.isEmpty()) {
        result = QStringLiteral("Screenshot");
    }
    return truncatedFilename(result);
}

QString ExportManager::autoIncrementFilename(const QString &baseName, const QString &extension, FileNameAlreadyUsedCheck isFileNameUsed) const
{
    QString result = truncatedFilename(baseName) + QLatin1String(".") + extension;
    if (!((this->*isFileNameUsed)(QUrl::fromUserInput(result)))) {
        return result;
    }

    QString fileNameFmt = truncatedFilename(baseName) + QStringLiteral("-%1.");
    for (quint64 i = 1; i < std::numeric_limits<quint64>::max(); i++) {
        result = fileNameFmt.arg(i) + extension;
        if (!((this->*isFileNameUsed)(QUrl::fromUserInput(result)))) {
            return result;
        }
    }

    // unlikely this will ever happen, but just in case we've run
    // out of numbers

    result = fileNameFmt.arg(QLatin1String("OVERFLOW-") + QString::number(QRandomGenerator::global()->bounded(10000)));
    return truncatedFilename(result) + extension;
}

QString ExportManager::imageFileSuffix(const QUrl &url) const
{
    QMimeDatabase mimedb;
    const QString type = mimedb.mimeTypeForUrl(url).preferredSuffix();

    if (type.isEmpty()) {
        return Settings::self()->defaultSaveImageFormat().toLower();
    }
    return type;
}

bool ExportManager::writeImage(QIODevice *device, const QByteArray &suffix)
{
    // In the documentation for QImageWriter, it is a bit ambiguous what "format" means.
    // From looking at how QImageWriter handles the built-in supported formats internally,
    // "format" basically means the file extension, not the mimetype.
    QImageWriter imageWriter(device, suffix);
    imageWriter.setQuality(Settings::self()->compressionQuality());
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

    if (!dir.mkpath(QStringLiteral("."))) {
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

    if (!m_tempDir) {
        m_tempDir = new QTemporaryDir(QDir::tempPath() + QDir::separator() + QStringLiteral("Spectacle.XXXXXX"));
    }
    if (m_tempDir && m_tempDir->isValid()) {
        // create the temporary file itself with normal file name and also unique one for this session
        // supports the use-case of creating multiple screenshots in a row
        // and exporting them to the same destination e.g. via clipboard,
        // where the temp file name is used as filename suggestion
        const QString baseFileName = m_tempDir->path() + QLatin1Char('/') + QUrl::fromLocalFile(formattedFilename()).fileName();

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
        const QString filenameExtension = Settings::self()->defaultSaveImageFormat().toLower();
        const QString mimetype = QMimeDatabase().mimeTypeForFile(QStringLiteral("~/fakefile.") + filenameExtension, QMimeDatabase::MatchExtension).name();
        QFileDialog dialog;
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.setFileMode(QFileDialog::AnyFile);
        QUrl dirUrl = url.adjusted(QUrl::RemoveFilename);
        if (!dirUrl.isValid()) {
            dirUrl = Settings::self()->lastSaveAsLocation().adjusted(QUrl::RemoveFilename);
        }
        dialog.setDirectoryUrl(dirUrl);
        dialog.selectFile(formattedFilename() + QStringLiteral(".") + filenameExtension);
        dialog.setDefaultSuffix(QStringLiteral(".") + filenameExtension);
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
        data->setImageData(m_saveImage);
        // "x-kde-force-image-copy" is handled by Klipper.
        // It ensures that the image is copied to Klipper even with the
        // "Non-text selection: Never save in history" setting selected in Klipper.
        data->setData(QStringLiteral("x-kde-force-image-copy"), QByteArray());
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
                url = Settings::self()->lastSaveLocation();
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

const QMap<QString, KLocalizedString> ExportManager::filenamePlaceholders{
    {QStringLiteral("%Y"), ki18nc("A placeholder in the user configurable filename will replaced by the specified value", "Year (4 digit)")},
    {QStringLiteral("%y"), ki18nc("A placeholder in the user configurable filename will replaced by the specified value", "Year (2 digit)")},
    {QStringLiteral("%M"), ki18nc("A placeholder in the user configurable filename will replaced by the specified value", "Month")},
    {QStringLiteral("%n"), ki18nc("A placeholder in the user configurable filename will replaced by the specified value", "Month (localized short name)")},
    {QStringLiteral("%N"), ki18nc("A placeholder in the user configurable filename will replaced by the specified value", "Month (localized long name)")},
    {QStringLiteral("%D"), ki18nc("A placeholder in the user configurable filename will replaced by the specified value", "Day")},
    {QStringLiteral("%H"), ki18nc("A placeholder in the user configurable filename will replaced by the specified value", "Hour")},
    {QStringLiteral("%m"), ki18nc("A placeholder in the user configurable filename will replaced by the specified value", "Minute")},
    {QStringLiteral("%S"), ki18nc("A placeholder in the user configurable filename will replaced by the specified value", "Second")},
    {QStringLiteral("%t"), ki18nc("A placeholder in the user configurable filename will replaced by the specified value", "Timezone")},
    {QStringLiteral("%T"), ki18nc("A placeholder in the user configurable filename will replaced by the specified value", "Window Title")},
    {QStringLiteral("%d"), ki18nc("A placeholder in the user configurable filename will replaced by the specified value", "Sequential numbering")},
    {QStringLiteral("%Nd"),
     ki18nc("A placeholder in the user configurable filename will replaced by the specified value", "Sequential numbering, padded out to N digits")},
};

#include "moc_ExportManager.cpp"
