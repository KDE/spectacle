/*
 *  SPDX-License-Identifier: GPL-2.0-only OR LGPL-2.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QDir>
#include <QFile>
#include <QTest>
#include <QUuid>

#include "ExportManager.h"
#include "CaptureModeModel.h"

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
    mExportManager->setTimestamp(QDateTime::fromString(QStringLiteral("2019-03-22T10:43:25"), Qt::ISODate));
    mExportManager->setWindowTitle(QStringLiteral("Spectacle"));
}

void FilenameTest::testStrings()
{
    QCOMPARE(mExportManager->formattedFilename(QStringLiteral("Screenshot")), QStringLiteral("Screenshot"));
    // empty string produces Screenshot per default
    QCOMPARE(mExportManager->formattedFilename(QString()), QStringLiteral("Screenshot"));
    // not a placeholder
    QCOMPARE(mExportManager->formattedFilename(QStringLiteral("%")), QStringLiteral("%"));
    QCOMPARE(mExportManager->formattedFilename(QStringLiteral("%K")), QStringLiteral("%K"));
}

void FilenameTest::testDateTokens()
{
    QCOMPARE(mExportManager->formattedFilename(QStringLiteral("%Y")), QStringLiteral("2019"));
    QCOMPARE(mExportManager->formattedFilename(QStringLiteral("%y")), QStringLiteral("19"));
    QCOMPARE(mExportManager->formattedFilename(QStringLiteral("%M")), QStringLiteral("03"));
    QCOMPARE(mExportManager->formattedFilename(QStringLiteral("%D")), QStringLiteral("22"));
    QCOMPARE(mExportManager->formattedFilename(QStringLiteral("%H")), QStringLiteral("10"));
    QCOMPARE(mExportManager->formattedFilename(QStringLiteral("%m")), QStringLiteral("43"));
    QCOMPARE(mExportManager->formattedFilename(QStringLiteral("%S")), QStringLiteral("25"));
}

void FilenameTest::testWindowTitle()
{
    mExportManager->setWindowTitle(QStringLiteral("Spectacle"));
    QCOMPARE(mExportManager->formattedFilename(QStringLiteral("%T")), QStringLiteral("Spectacle"));
    QCOMPARE(mExportManager->formattedFilename(QStringLiteral("Before%TAfter")), QStringLiteral("BeforeSpectacleAfter"));
    mExportManager->setWindowTitle({});
    // Empty String produces Screenshot
    QCOMPARE(mExportManager->formattedFilename(QStringLiteral("%T")), QStringLiteral("Screenshot"));
    QCOMPARE(mExportManager->formattedFilename(QStringLiteral("Before%TAfter")), QStringLiteral("BeforeAfter"));
    QCOMPARE(mExportManager->formattedFilename(QStringLiteral("Before_%T_After")), QStringLiteral("Before_After"));
}

void FilenameTest::testNumbering()
{
    QString BaseName = QLatin1String("spectacle_test_") + QUuid::createUuid().toString();
    QCOMPARE(mExportManager->formattedFilename(BaseName + QStringLiteral("_%d")), BaseName + QStringLiteral("_1"));
    QCOMPARE(mExportManager->formattedFilename(BaseName + QStringLiteral("_%1d")), BaseName + QStringLiteral("_1"));
    QCOMPARE(mExportManager->formattedFilename(BaseName + QStringLiteral("_%2d")), BaseName + QStringLiteral("_01"));
    QCOMPARE(mExportManager->formattedFilename(BaseName + QStringLiteral("_%3d")), BaseName + QStringLiteral("_001"));
    QCOMPARE(mExportManager->formattedFilename(BaseName + QStringLiteral("_%4d")), BaseName + QStringLiteral("_0001"));
    QCOMPARE(mExportManager->formattedFilename(BaseName + QStringLiteral("_%d_%2d_%3d")), BaseName + QStringLiteral("_1_01_001"));

    QFile file(QDir(mExportManager->defaultSaveLocation()).filePath(BaseName + QStringLiteral("_1.png")));
    file.open(QIODevice::WriteOnly);
    file.close();
    QCOMPARE(mExportManager->formattedFilename(BaseName + QStringLiteral("_%d")), BaseName + QStringLiteral("_2"));
    file.remove();
    file.setFileName(QDir(mExportManager->defaultSaveLocation()).filePath(BaseName + QStringLiteral("_1_01_001")));
    file.open(QIODevice::WriteOnly);
    file.close();
    QCOMPARE(mExportManager->formattedFilename(BaseName + QStringLiteral("_%d_%2d_%3d")), BaseName + QStringLiteral("_2_02_002"));
    file.remove();
}

void FilenameTest::testCombined()
{
    mExportManager->setWindowTitle(QStringLiteral("Spectacle"));
    QCOMPARE(mExportManager->formattedFilename(QStringLiteral("App_%T_Date_%Y%M%D_Time_%H:%m:%S%F")),
             QStringLiteral("App_Spectacle_Date_20190322_Time_10:43:25%F"));
    mExportManager->setWindowTitle({});
    QCOMPARE(mExportManager->formattedFilename(QStringLiteral("App_%T_Date_%Y%M%D_Time_%H:%m:%S%F")), QStringLiteral("App_Date_20190322_Time_10:43:25%F"));
}

QTEST_GUILESS_MAIN(FilenameTest)

#include "FilenameTest.moc"
