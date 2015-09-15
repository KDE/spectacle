/*
 *  Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include <QApplication>
#include <QIcon>
#include <QObject>
#include <QString>
#include <QCommandLineParser>
#include <QDebug>

#include <KAboutData>
#include <KLocalizedString>

#include "KSCore.h"
#include "Config.h"

int main(int argc, char **argv)
{
    // set up the application

    QApplication app(argc, argv);

    app.setOrganizationDomain("kde.org");
    app.setApplicationName("kapture");
    app.setWindowIcon(QIcon::fromTheme("ksnapshot"));

    app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    // set up the about data

    KLocalizedString::setApplicationDomain("kapture");
    KAboutData aboutData("kapture",
                         i18n("Kapture"),
                         KAPTURE_VERSION,
                         i18n("KDE Screenshot Utility"),
                         KAboutLicense::GPL_V2,
                         i18n("(C) 2015 Boudhayan Gupta"));
    aboutData.addAuthor("Boudhayan Gupta", QString(), "bgupta@kde.org");
    KAboutData::setApplicationData(aboutData);

    // set up the command line options parser

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);

    parser.addOptions({
        {{"f", "fullscreen"},        i18n("Capture the entire desktop (default)")},
        {{"m", "current"},           i18n("Capture the current monitor")},
        {{"a", "activewindow"},      i18n("Capture the active window")},
        {{"u", "windowundercursor"}, i18n("Capture the window currently under the cursor, including parents of pop-up menus")},
        {{"t", "transientonly"},     i18n("Capture the window currently under the cursor, excluding parents of pop-up menus")},
        {{"r", "region"},            i18n("Capture a rectangular region of the screen")},
        {{"b", "background"},        i18n("Take a screenshot and exit without showing the GUI")},
        {{"n", "notify"},            i18n("In background mode, pop up a notification when the screenshot is taken")},
        {{"c", "clipboard"},         i18n("In background mode, send image to clipboard without saving to file")},
        {{"o", "output"},            i18n("In background mode, save image to specified file"), "fileName"},
        {{"d", "delay"},             i18n("In background mode, delay before taking the shot (in milliseconds)"), "delayMsec"},
        {{"w", "onclick"},           i18n("Wait for a click before taking screenshot. Invalidates delay")}
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
    } else if (parser.isSet("windowundercursor")) {
        grabMode = ImageGrabber::TransientWithParent;
    } else if (parser.isSet("transientonly")) {
        grabMode = ImageGrabber::WindowUnderCursor;
    }

    // are we running in background mode?

    bool backgroundMode = false;
    bool sendToClipboard = false;
    bool notify = false;
    qint64 delayMsec = 0;
    QString fileName = QString();

    if (parser.isSet("background")) {
        backgroundMode = true;
        app.setQuitOnLastWindowClosed(false);

        if (parser.isSet("notify")) {
            notify = true;
        }

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

    KSCore genie(backgroundMode, grabMode, fileName, delayMsec, sendToClipboard, notify);
    QObject::connect(&genie, &KSCore::allDone, qApp, &QApplication::quit);

    return app.exec();
}
