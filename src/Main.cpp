#include <QApplication>
#include <QIcon>
#include <QObject>
#include <QString>
#include <QCommandLineParser>
#include <QDebug>

#include <KAboutData>
#include <KLocalizedString>

#include "KScreenGenie.h"
#include "ImageGrabber.h"
#include "Config.h"

int main(int argc, char **argv)
{
    // set up the application

    QApplication app(argc, argv);

    app.setOrganizationDomain("kde.org");
    app.setApplicationName("kscreengenie");
    app.setWindowIcon(QIcon::fromTheme("ksnapshot"));

    app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    // set up the about data

    KLocalizedString::setApplicationDomain("kscreengenie");
    KAboutData aboutData("kscreengenie",
                         i18n("KScreenGenie"),
                         KSG_VERSION,
                         i18n("KDE Screenshot Utility"),
                         KAboutLicense::GPL_V2,
                         i18n("(C) 2015 Boudhayan Gupta"));
    aboutData.addAuthor("Boudhayan Gupta", QString(), "me@BaloneyGeek.com");
    KAboutData::setApplicationData(aboutData);

    // set up the command line options parser

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);

    parser.addOptions({
        {{"f", "fullscreen"},   i18n("Capture the entire desktop (default)")},
        {{"m", "current"},      i18n("Capture the current monitor")},
        {{"a", "activewindow"}, i18n("Capture the active window")},
        {{"r", "region"},       i18n("Capture a rectangular region of the screen")},
        {{"b", "background"},   i18n("Take a screenshot and exit without showing the GUI")},
        {{"c", "clipboard"},    i18n("In background mode, send image to clipboard without saving to file")},
        {{"o", "output"},       i18n("In background mode, save image to specified file"), "fileName"},
        {{"d", "delay"},        i18n("In background mode, delay before taking the shot (in milliseconds)"), "delayMsec"},
        {{"w", "onclick"},      i18n("Wait for a click before taking screenshot. Invalidates delay")}
    });

    parser.process(app);
    aboutData.processCommandLine(&parser);

    // extract the capture mode

    ImageGrabber::GrabMode grabMode = ImageGrabber::FullScreen;
    if (parser.isSet("current")) {
        grabMode = ImageGrabber::CurrentScreen;
    } else if (parser.isSet("activewindow")) {
        grabMode = ImageGrabber::ActiveWindow;
    } else if (parser.isSet("region")) {
        grabMode = ImageGrabber::RectangularRegion;
    }

    // are we running in background mode?

    bool backgroundMode = false;
    bool sendToClipboard = false;
    qint64 delayMsec = 0;
    QString fileName = QString();

    if (parser.isSet("background")) {
        backgroundMode = true;

        if (parser.isSet("output")) {
            fileName = parser.value("output");
        }

        if (parser.isSet("delay")) {
            bool ok = false;
            qint64 delayValue = parser.value("delay").toLongLong(&ok);
            if (ok) {
                delayMsec = delayValue;
            }
        }

        if (parser.isSet("onclick")) {
            delayMsec = -1;
        }

        if (parser.isSet("clipboard")) {
            sendToClipboard = true;
        }
    }

    // release the kraken

    KScreenGenie genie(backgroundMode, grabMode, fileName, delayMsec, sendToClipboard);
    QObject::connect(&genie, &KScreenGenie::allDone, qApp, &QApplication::quit);

    return app.exec();
}
