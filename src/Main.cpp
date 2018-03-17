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

#include "Config.h"
#include "SpectacleCore.h"
#include "SpectacleDBusAdapter.h"

#include <KAboutData>
#include <KDBusService>
#include <KLocalizedString>

#include <QCommandLineParser>
#include <QDBusConnection>

int main(int argc, char **argv)
{
    // set up the application

    QApplication app(argc, argv);

    app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    KLocalizedString::setApplicationDomain("spectacle");

    KAboutData aboutData(QStringLiteral("spectacle"),
                         i18n("Spectacle"),
                         QStringLiteral(SPECTACLE_VERSION) + QStringLiteral(" - ") + QStringLiteral(SPECTACLE_CODENAME),
                         i18n("KDE Screenshot Utility"),
                         KAboutLicense::GPL_V2,
                         i18n("(C) 2015 Boudhayan Gupta"));
    aboutData.addAuthor(QStringLiteral("Boudhayan Gupta"), QString(), QStringLiteral("bgupta@kde.org"));
    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));
    KAboutData::setApplicationData(aboutData);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("spectacle")));

    // set up the command line options parser

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);

    parser.addOptions({
        {{QStringLiteral("f"), QStringLiteral("fullscreen")},        i18n("Capture the entire desktop (default)")},
        {{QStringLiteral("m"), QStringLiteral("current")},           i18n("Capture the current monitor")},
        {{QStringLiteral("a"), QStringLiteral("activewindow")},      i18n("Capture the active window")},
        {{QStringLiteral("u"), QStringLiteral("windowundercursor")}, i18n("Capture the window currently under the cursor, including parents of pop-up menus")},
        {{QStringLiteral("t"), QStringLiteral("transientonly")},     i18n("Capture the window currently under the cursor, excluding parents of pop-up menus")},
        {{QStringLiteral("r"), QStringLiteral("region")},            i18n("Capture a rectangular region of the screen")},
        {{QStringLiteral("g"), QStringLiteral("gui")},               i18n("Start in GUI mode (default)")},
        {{QStringLiteral("b"), QStringLiteral("background")},        i18n("Take a screenshot and exit without showing the GUI")},
        {{QStringLiteral("s"), QStringLiteral("dbus")},              i18n("Start in DBus-Activation mode")},
        {{QStringLiteral("n"), QStringLiteral("nonotify")},          i18n("In background mode, do not pop up a notification when the screenshot is taken")},
        {{QStringLiteral("o"), QStringLiteral("output")},            i18n("In background mode, save image to specified file"), QStringLiteral("fileName")},
        {{QStringLiteral("d"), QStringLiteral("delay")},             i18n("In background mode, delay before taking the shot (in milliseconds)"), QStringLiteral("delayMsec")},
        {{QStringLiteral("w"), QStringLiteral("onclick")},           i18n("Wait for a click before taking screenshot. Invalidates delay")}
    });

    parser.process(app);
    aboutData.processCommandLine(&parser);

    // extract the capture mode

    ImageGrabber::GrabMode grabMode = ImageGrabber::FullScreen;
    if (parser.isSet(QStringLiteral("current"))) {
        grabMode = ImageGrabber::CurrentScreen;
    } else if (parser.isSet(QStringLiteral("activewindow"))) {
        grabMode = ImageGrabber::ActiveWindow;
    } else if (parser.isSet(QStringLiteral("region"))) {
        grabMode = ImageGrabber::RectangularRegion;
    } else if (parser.isSet(QStringLiteral("windowundercursor"))) {
        grabMode = ImageGrabber::TransientWithParent;
    } else if (parser.isSet(QStringLiteral("transientonly"))) {
        grabMode = ImageGrabber::WindowUnderCursor;
    }

    // are we running in background or dbus mode?

    SpectacleCore::StartMode startMode = SpectacleCore::GuiMode;
    bool notify = true;
    qint64 delayMsec = 0;
    QString fileName = QString();

    if (parser.isSet(QStringLiteral("background"))) {
        startMode = SpectacleCore::BackgroundMode;
    } else if (parser.isSet(QStringLiteral("dbus"))) {
        startMode = SpectacleCore::DBusMode;
    }

    switch (startMode) {
    case SpectacleCore::BackgroundMode:
        if (parser.isSet(QStringLiteral("nonotify"))) {
            notify = false;
        }

        if (parser.isSet(QStringLiteral("output"))) {
            fileName = parser.value(QStringLiteral("output"));
        }

        if (parser.isSet(QStringLiteral("delay"))) {
            bool ok = false;
            qint64 delayValue = parser.value(QStringLiteral("delay")).toLongLong(&ok);
            if (ok) {
                delayMsec = delayValue;
            }
        }

        if (parser.isSet(QStringLiteral("onclick"))) {
            delayMsec = -1;
        }

        app.setQuitOnLastWindowClosed(false);
        break;

    case SpectacleCore::DBusMode:
        app.setQuitOnLastWindowClosed(false);
    case SpectacleCore::GuiMode:
        break;
    }

    // release the kraken

    SpectacleCore core(startMode, grabMode, fileName, delayMsec, notify);
    QObject::connect(&core, &SpectacleCore::allDone, qApp, &QApplication::quit);

    // create the dbus connections

    new KDBusService(KDBusService::Multiple, &core);

    SpectacleDBusAdapter *dbusAdapter = new SpectacleDBusAdapter(&core);
    QObject::connect(&core, &SpectacleCore::grabFailed, dbusAdapter, &SpectacleDBusAdapter::ScreenshotFailed);
    QObject::connect(ExportManager::instance(), &ExportManager::imageSaved, [&](const QUrl savedAt) {
        emit dbusAdapter->ScreenshotTaken(savedAt.toLocalFile());
    });

    QDBusConnection::sessionBus().registerObject(QStringLiteral("/"), &core);
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.Spectacle"));

    // fire it up

    return app.exec();
}
