#include <QApplication>
#include <QObject>
#include <QString>
#include <QCommandLineParser>
#include <QDebug>

#include <KAboutData>
#include <KLocalizedString>

#include "KScreenGenie.h"
#include "ImageGrabber.h"

int main(int argc, char **argv)
{
    // set up the application

    QApplication app(argc, argv);

    app.setOrganizationDomain("kde.org");
    app.setApplicationName("kscreengenie");
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    // set up the about data

    KLocalizedString::setApplicationDomain("kscreengenie");
    KAboutData aboutData("kscreengenie",
                         i18n("KScreenGenie"),
                         "0.0.95",
                         i18n("KDE Screenshot Utility"),
                         KAboutLicense::LGPL,
                         i18n("(C) 2015 Boudhayan Gupta"));
    aboutData.addAuthor("Boudhayan Gupta", QString(), "me@BaloneyGeek.com");
    KAboutData::setApplicationData(aboutData);

    // set up the command line options parser

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);

    parser.addOptions({
        {{"c", "current"},      i18n("Capture the window under the cursor at startup")},
        {{"f", "fullscreen"},   i18n("Capture the entire desktop (default)")},
        {{"a", "activewindow"}, i18n("Capture the active window")},
        {{"e", "edit"},         i18n("Edit the screenshot after taking it (crop, mask, etc)")},
        {{"b", "background"},   i18n("Take a screenshot and exit without showing the GUI")}
    });

    parser.process(app);
    aboutData.processCommandLine(&parser);

    // extract the capture mode

    ImageGrabber::GrabMode grabMode = ImageGrabber::FullScreen;
    if (parser.isSet("current")) {
        grabMode = ImageGrabber::CurrentScreen;
    } else if (parser.isSet("activewindow")) {
        grabMode = ImageGrabber::ActiveWindow;
    }

    // extract the editor mode (do we want to manipulate the image post capture?)

    bool startEditor = false;
    if (parser.isSet("edit")) {
        startEditor = true;
    }

    // are we running in background mode?

    bool backgroundMode = false;
    if (parser.isSet("background")) {
        backgroundMode = true;
    }

    // release the kraken

    KScreenGenie genie(backgroundMode, startEditor, grabMode);
    QObject::connect(&genie, &KScreenGenie::allDone, qApp, &QApplication::quit);

    return app.exec();
}
