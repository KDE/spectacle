/*
 *  SPDX-License-Identifier: GPL-2.0-only OR LGPL-2.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QDir>
#include <QFile>
#include <QLocale>
#include <QTest>
#include <QUuid>

#include "ExportManager.h"
#include "CaptureModeModel.h"

using namespace Qt::StringLiterals;

class FilenameTest : public QObject
{
    Q_OBJECT

private:
    ExportManager *mExportManager;
    QDateTime timestamp;

private Q_SLOTS:

    void initTestCase();
    void testStrings();
    void testDateTokens();
    void testWindowTitle();
    void testNumbering();
    void testCombined();
};

void FilenameTest::initTestCase()
{
    mExportManager = ExportManager::instance();
    timestamp = QDateTime::fromString(u"2019-03-22T20:43:25Z"_s, Qt::ISODate);
    mExportManager->setTimestamp(timestamp);
    mExportManager->setWindowTitle(u"Spectacle"_s);
}

void FilenameTest::testStrings()
{
    QCOMPARE(mExportManager->formattedFilename(u"Screenshot"_s), u"Screenshot"_s);
    // empty string produces Screenshot per default
    QCOMPARE(mExportManager->formattedFilename({}), u"Screenshot"_s);
    // not a placeholder
    QCOMPARE(mExportManager->formattedFilename(u"<"_s), u"<"_s);
    QCOMPARE(mExportManager->formattedFilename(u">"_s), u">"_s);
    QCOMPARE(mExportManager->formattedFilename(u"<>"_s), u"<>"_s);
    QCOMPARE(mExportManager->formattedFilename(u"<notaplaceholder>"_s), u"<notaplaceholder>"_s);
}

void FilenameTest::testDateTokens()
{
    const auto &placeholders = mExportManager->filenamePlaceholders;
    const auto &locale = QLocale::system();
    for (auto it = placeholders.cbegin(); it != placeholders.cend(); ++it) {
        using Flag = ExportManager::Placeholder::Flag;
        if (it->flags.testFlag(Flag::QDateTime)) {
            QCOMPARE(mExportManager->formattedFilename(it->plainKey), locale.toString(timestamp, it->baseKey));
        }
    }
    QCOMPARE(mExportManager->formattedFilename(u"<h>"_s), u"8"_s);
    QCOMPARE(mExportManager->formattedFilename(u"<hh>"_s), u"08"_s);
    QCOMPARE(mExportManager->formattedFilename(u"<UnixTime>"_s), QString::number(timestamp.toSecsSinceEpoch()));
}

void FilenameTest::testWindowTitle()
{
    enum : std::size_t {
        WithTitle,
        WithoutTitle,
        ArraySize,
    };
    static const QMap<QString, std::array<QString, ArraySize>> comparisons{
        {u"<title>"_s, {u"Spectacle"_s, u"Screenshot"_s}}, // Empty String produces Screenshot
        {u"Before<title>After"_s, {u"BeforeSpectacleAfter"_s, u"BeforeAfter"_s}},
        {u"Before_<title>_After"_s, {u"Before_Spectacle_After"_s, u"Before_After"_s}},
        {u"Before_<title>"_s, {u"Before_Spectacle"_s, u"Before"_s}},
        {u"<title>_After"_s, {u"Spectacle_After"_s, u"After"_s}},
        {u"<mm><title><ss>"_s, {u"43Spectacle25"_s, u"4325"_s}},
        {u"<mm>_<title>_<ss>"_s, {u"43_Spectacle_25"_s, u"43_25"_s}},
        {u"<mm>/<title>/<title>/<ss>"_s, {u"43/Spectacle/Spectacle/25"_s, u"43/25"_s}},
        {u"<mm>_<title>/<title>_<ss>"_s, {u"43_Spectacle/Spectacle_25"_s, u"43/25"_s}},
        {u"<mm>_/<title>/_<ss>"_s, {u"43_/Spectacle/_25"_s, u"43_/_25"_s}},
        {u"<mm>_/_<title>_/_<ss>"_s, {u"43_/_Spectacle_/_25"_s, u"43_/_25"_s}},
    };
    mExportManager->setWindowTitle(u"Spectacle"_s);
    for (auto it = comparisons.cbegin(); it != comparisons.cend(); ++it) {
        QCOMPARE(mExportManager->formattedFilename(it.key()), it.value()[WithTitle]);
    }
    mExportManager->setWindowTitle({});
    for (auto it = comparisons.cbegin(); it != comparisons.cend(); ++it) {
        QCOMPARE(mExportManager->formattedFilename(it.key()), it.value()[WithoutTitle]);
    }
}

void FilenameTest::testNumbering()
{
    QString BaseName = u"spectacle_test_" + QUuid::createUuid().toString();
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_<#>"_s), BaseName + u"_1"_s);
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_<##>"_s), BaseName + u"_01"_s);
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_<###>"_s), BaseName + u"_001"_s);
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_<####>"_s), BaseName + u"_0001"_s);
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_<#>_<##>_<###>"_s), BaseName + u"_1_01_001"_s);

    QFile file(QDir(mExportManager->defaultSaveLocation()).filePath(BaseName + u"_3.png"_s));
    file.open(QIODevice::WriteOnly);
    file.close();
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_<#>"_s), BaseName + u"_4"_s);
    file.remove();
    file.setFileName(QDir(mExportManager->defaultSaveLocation()).filePath(BaseName + u"_0008"_s));
    file.open(QIODevice::WriteOnly);
    file.close();
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_<####>"_s), BaseName + u"_0009"_s);
    file.remove();
    file.setFileName(QDir(mExportManager->defaultSaveLocation()).filePath(BaseName + u"_7_07_007"_s));
    file.open(QIODevice::WriteOnly);
    file.close();
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_<#>_<##>_<###>"_s), BaseName + u"_8_08_008"_s);
    file.remove();
}

void FilenameTest::testCombined()
{
    mExportManager->setWindowTitle(u"Spectacle"_s);
    static const auto filenameTemplate = u"App_<title>/Date_<yyyy><MM><dd>_Time_<hh>:<mm>:<ss><AP><notaplaceholder>"_s;
    QCOMPARE(mExportManager->formattedFilename(filenameTemplate),
             u"App_Spectacle/Date_20190322_Time_08:43:25PM<notaplaceholder>"_s);
    mExportManager->setWindowTitle({});
    QCOMPARE(mExportManager->formattedFilename(filenameTemplate),
             u"App/Date_20190322_Time_08:43:25PM<notaplaceholder>"_s);
}

QTEST_GUILESS_MAIN(FilenameTest)

#include "FilenameTest.moc"
