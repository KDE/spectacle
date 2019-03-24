#include <QDir>
#include <QFile>
#include <QtTest>
#include <QUuid>

#include "ExportManager.h"

class FilenameTest : public QObject
{
    Q_OBJECT
private:
    ExportManager* em;
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
    em = ExportManager::instance();
    em->setTimestamp(QDateTime::fromString(QStringLiteral("2019-03-22T10:43:25"), Qt::ISODate));
    em->setWindowTitle(QStringLiteral("Spectacle"));
}

void FilenameTest::testStrings()
{
    QCOMPARE(em->formatFilename(QStringLiteral("Screenshot")), QStringLiteral("Screenshot"));
    // empty string produces Screenshot per default
    QCOMPARE(em->formatFilename(QStringLiteral("")), QStringLiteral("Screenshot"));
    // not a placeholder
    QCOMPARE(em->formatFilename(QStringLiteral("%")), QStringLiteral("%"));
    QCOMPARE(em->formatFilename(QStringLiteral("%K")), QStringLiteral("%K"));
}

void FilenameTest::testDateTokens()
{
    QCOMPARE(em->formatFilename(QStringLiteral("%Y")), QStringLiteral("2019"));
    QCOMPARE(em->formatFilename(QStringLiteral("%y")), QStringLiteral("19"));
    QCOMPARE(em->formatFilename(QStringLiteral("%M")), QStringLiteral("03"));
    QCOMPARE(em->formatFilename(QStringLiteral("%D")), QStringLiteral("22"));
    QCOMPARE(em->formatFilename(QStringLiteral("%H")), QStringLiteral("10"));
    QCOMPARE(em->formatFilename(QStringLiteral("%m")), QStringLiteral("43"));
    QCOMPARE(em->formatFilename(QStringLiteral("%S")), QStringLiteral("25"));
}

void FilenameTest::testWindowTitle()
{
    em->setGrabMode(ImageGrabber::ActiveWindow);
    QCOMPARE(em->formatFilename(QStringLiteral("%T")), QStringLiteral("Spectacle"));
    QCOMPARE(em->formatFilename(QStringLiteral("Before%TAfter")),
             QStringLiteral("BeforeSpectacleAfter"));
    em->setGrabMode(ImageGrabber::FullScreen);
    //Empty String produces Screenshot
    QCOMPARE(em->formatFilename(QStringLiteral("%T")), QStringLiteral("Screenshot"));
    QCOMPARE(em->formatFilename(QStringLiteral("Before%TAfter")), QStringLiteral("BeforeAfter"));
    QCOMPARE(em->formatFilename(QStringLiteral("Before_%T_After")), QStringLiteral("Before_After"));
}

void FilenameTest::testNumbering()
{
    QString baseName = QStringLiteral("spectacle_test_")+ QUuid::createUuid().toString();
    QCOMPARE(em->formatFilename(baseName+QStringLiteral("_%d")), baseName+QStringLiteral("_1"));
    QCOMPARE(em->formatFilename(baseName+QStringLiteral("_%1d")), baseName+QStringLiteral("_1"));
    QCOMPARE(em->formatFilename(baseName+QStringLiteral("_%2d")), baseName+QStringLiteral("_01"));
    QCOMPARE(em->formatFilename(baseName+QStringLiteral("_%3d")), baseName+QStringLiteral("_001"));
    QCOMPARE(em->formatFilename(baseName+QStringLiteral("_%4d")), baseName+QStringLiteral("_0001"));
    QCOMPARE(em->formatFilename(baseName+QStringLiteral("_%d_%2d_%3d")),
             baseName+QStringLiteral("_1_01_001"));
    QFile file(QDir(em->defaultSaveLocation()).filePath(baseName + QStringLiteral("_1.png")));
    file.open(QIODevice::WriteOnly);
    QCOMPARE(em->formatFilename(baseName+QStringLiteral("_%d")), baseName+QStringLiteral("_2"));
    file.remove();
}

void FilenameTest::testCombined()
{
    em->setGrabMode(ImageGrabber::ActiveWindow);
    QCOMPARE(em->formatFilename(QStringLiteral("App_%T_Date_%Y%M%D_Time_%H:%m:%S%F")),
             QStringLiteral("App_Spectacle_Date_20190322_Time_10:43:25%F"));
    em->setGrabMode(ImageGrabber::FullScreen);
    QCOMPARE(em->formatFilename(QStringLiteral("App_%T_Date_%Y%M%D_Time_%H:%m:%S%F")),
             QStringLiteral("App_Date_20190322_Time_10:43:25%F"));
}

QTEST_MAIN(FilenameTest)

#include "FilenameTest.moc"
