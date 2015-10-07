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
#include <QDBusConnection>
#include <QDebug>

#include <KAboutData>
#include <KLocalizedString>
#include <KDBusService>

#include "SpectacleCore.h"
#include "SpectacleDBusAdapter.h"
#include "Config.h"

int main(int argc, char **argv)
{
    // set up the application

    QApplication app(argc, argv);

    app.setOrganizationDomain("kde.org");
    app.setApplicationName("spectacle");
    app.setWindowIcon(QIcon::fromTheme("spectacle"));

    app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    // set up the about data

    KLocalizedString::setApplicationDomain("spectacle");
    KAboutData aboutData("spectacle",
                         i18n("Spectacle"),
                         SPECTACLE_VERSION,
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
        {{"g", "gui"},               i18n("Start in GUI mode (default)")},
        {{"b", "background"},        i18n("Take a screenshot and exit without showing the GUI")},
        {{"s", "dbus"},              i18n("Start in DBus-Activation mode")},
        {{"n", "nonotify"},          i18n("In background mode, do not pop up a notification when the screenshot is taken")},
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

    // are we running in background or dbus mode?

    SpectacleCore::StartMode startMode = SpectacleCore::GuiMode;
    bool sendToClipboard = false;
    bool notify = true;
    qint64 delayMsec = 0;
    QString fileName = QString();

    if (parser.isSet("background")) {
        startMode = SpectacleCore::BackgroundMode;
    } else if (parser.isSet("dbus")) {
        startMode = SpectacleCore::DBusMode;
    }

    switch (startMode) {
    case SpectacleCore::BackgroundMode:
        if (parser.isSet("nonotify")) {
            notify = false;
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
    case SpectacleCore::DBusMode:
        app.setQuitOnLastWindowClosed(false);
    case SpectacleCore::GuiMode:
        break;
    }

    // release the kraken

    SpectacleCore core(startMode, grabMode, fileName, delayMsec, sendToClipboard, notify);
    QObject::connect(&core, &SpectacleCore::allDone, qApp, &QApplication::quit);

    // create the dbus connections

    new KDBusService(KDBusService::Multiple, &core);

    SpectacleDBusAdapter *dbusAdapter = new SpectacleDBusAdapter(&core);
    QObject::connect(&core, static_cast<void (SpectacleCore::*)(QString)>(&SpectacleCore::imageSaved), dbusAdapter, &SpectacleDBusAdapter::ScreenshotTaken);
    QObject::connect(&core, &SpectacleCore::grabFailed, dbusAdapter, &SpectacleDBusAdapter::ScreenshotFailed);

    QDBusConnection::sessionBus().registerObject("/", &core);
    QDBusConnection::sessionBus().registerService("org.kde.Spectacle");

    // fire it up

    return app.exec();
}
