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
}

void FilenameTest::testWindowTitle()
{
    mExportManager->setWindowTitle(u"Spectacle"_s);
    QCOMPARE(mExportManager->formattedFilename(u"<title>"_s), u"Spectacle"_s);
    QCOMPARE(mExportManager->formattedFilename(u"Before<title>After"_s), u"BeforeSpectacleAfter"_s);
    mExportManager->setWindowTitle({});
    // Empty String produces Screenshot
    QCOMPARE(mExportManager->formattedFilename(u"<title>"_s), u"Screenshot"_s);
    QCOMPARE(mExportManager->formattedFilename(u"Before<title>After"_s), u"BeforeAfter"_s);
    QCOMPARE(mExportManager->formattedFilename(u"Before_<title>_After"_s), u"Before_After"_s);
}

void FilenameTest::testNumbering()
{
    QString BaseName = u"spectacle_test_" + QUuid::createUuid().toString();
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_<#>"_s), BaseName + u"_1"_s);
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_<##>"_s), BaseName + u"_01"_s);
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_<###>"_s), BaseName + u"_001"_s);
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_<####>"_s), BaseName + u"_0001"_s);
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_<#>_<##>_<###>"_s), BaseName + u"_1_01_001"_s);

    QFile file(QDir(mExportManager->defaultSaveLocation()).filePath(BaseName + u"_1.png"_s));
    file.open(QIODevice::WriteOnly);
    file.close();
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_<#>"_s), BaseName + u"_2"_s);
    file.remove();
    file.setFileName(QDir(mExportManager->defaultSaveLocation()).filePath(BaseName + u"_1_01_001"_s));
    file.open(QIODevice::WriteOnly);
    file.close();
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_<#>_<##>_<###>"_s), BaseName + u"_2_02_002"_s);
    file.remove();
}

void FilenameTest::testCombined()
{
    mExportManager->setWindowTitle(u"Spectacle"_s);
    QCOMPARE(mExportManager->formattedFilename(u"App_<title>_Date_<yyyy><MM><dd>_Time_<HH>:<mm>:<ss><notaplaceholder>"_s),
             u"App_Spectacle_Date_20190322_Time_20:43:25<notaplaceholder>"_s);
    mExportManager->setWindowTitle({});
    QCOMPARE(mExportManager->formattedFilename(u"App_<title>_Date_<yyyy><MM><dd>_Time_<HH>:<mm>:<ss><notaplaceholder>"_s),
             u"App_Date_20190322_Time_20:43:25<notaplaceholder>"_s);
}

QTEST_GUILESS_MAIN(FilenameTest)

#include "FilenameTest.moc"
