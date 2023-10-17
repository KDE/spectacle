/*
 *  SPDX-License-Identifier: GPL-2.0-only OR LGPL-2.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QDir>
#include <QFile>
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
    mExportManager->setTimestamp(QDateTime::fromString(u"2019-03-22T10:43:25"_s, Qt::ISODate));
    mExportManager->setWindowTitle(u"Spectacle"_s);
}

void FilenameTest::testStrings()
{
    QCOMPARE(mExportManager->formattedFilename(u"Screenshot"_s), u"Screenshot"_s);
    // empty string produces Screenshot per default
    QCOMPARE(mExportManager->formattedFilename({}), u"Screenshot"_s);
    // not a placeholder
    QCOMPARE(mExportManager->formattedFilename(u"%"_s), u"%"_s);
    QCOMPARE(mExportManager->formattedFilename(u"%K"_s), u"%K"_s);
}

void FilenameTest::testDateTokens()
{
    QCOMPARE(mExportManager->formattedFilename(u"%Y"_s), u"2019"_s);
    QCOMPARE(mExportManager->formattedFilename(u"%y"_s), u"19"_s);
    QCOMPARE(mExportManager->formattedFilename(u"%M"_s), u"03"_s);
    QCOMPARE(mExportManager->formattedFilename(u"%D"_s), u"22"_s);
    QCOMPARE(mExportManager->formattedFilename(u"%H"_s), u"10"_s);
    QCOMPARE(mExportManager->formattedFilename(u"%m"_s), u"43"_s);
    QCOMPARE(mExportManager->formattedFilename(u"%S"_s), u"25"_s);
}

void FilenameTest::testWindowTitle()
{
    mExportManager->setWindowTitle(u"Spectacle"_s);
    QCOMPARE(mExportManager->formattedFilename(u"%T"_s), u"Spectacle"_s);
    QCOMPARE(mExportManager->formattedFilename(u"Before%TAfter"_s), u"BeforeSpectacleAfter"_s);
    mExportManager->setWindowTitle({});
    // Empty String produces Screenshot
    QCOMPARE(mExportManager->formattedFilename(u"%T"_s), u"Screenshot"_s);
    QCOMPARE(mExportManager->formattedFilename(u"Before%TAfter"_s), u"BeforeAfter"_s);
    QCOMPARE(mExportManager->formattedFilename(u"Before_%T_After"_s), u"Before_After"_s);
}

void FilenameTest::testNumbering()
{
    QString BaseName = u"spectacle_test_" + QUuid::createUuid().toString();
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_%d"_s), BaseName + u"_1"_s);
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_%1d"_s), BaseName + u"_1"_s);
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_%2d"_s), BaseName + u"_01"_s);
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_%3d"_s), BaseName + u"_001"_s);
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_%4d"_s), BaseName + u"_0001"_s);
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_%d_%2d_%3d"_s), BaseName + u"_1_01_001"_s);

    QFile file(QDir(mExportManager->defaultSaveLocation()).filePath(BaseName + u"_1.png"_s));
    file.open(QIODevice::WriteOnly);
    file.close();
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_%d"_s), BaseName + u"_2"_s);
    file.remove();
    file.setFileName(QDir(mExportManager->defaultSaveLocation()).filePath(BaseName + u"_1_01_001"_s));
    file.open(QIODevice::WriteOnly);
    file.close();
    QCOMPARE(mExportManager->formattedFilename(BaseName + u"_%d_%2d_%3d"_s), BaseName + u"_2_02_002"_s);
    file.remove();
}

void FilenameTest::testCombined()
{
    mExportManager->setWindowTitle(u"Spectacle"_s);
    QCOMPARE(mExportManager->formattedFilename(u"App_%T_Date_%Y%M%D_Time_%H:%m:%S%F"_s),
             u"App_Spectacle_Date_20190322_Time_10:43:25%F"_s);
    mExportManager->setWindowTitle({});
    QCOMPARE(mExportManager->formattedFilename(u"App_%T_Date_%Y%M%D_Time_%H:%m:%S%F"_s),
             u"App_Date_20190322_Time_10:43:25%F"_s);
}

QTEST_GUILESS_MAIN(FilenameTest)

#include "FilenameTest.moc"
