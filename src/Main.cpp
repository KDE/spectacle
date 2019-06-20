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
#include "SpectacleCommon.h"
#include "SpectacleCore.h"
#include "SpectacleDBusAdapter.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>

#include <KAboutData>
#include <KDBusService>
#include <KLocalizedString>

int main(int argc, char **argv)
{
    // set up the application

    QApplication lApp(argc, argv);

    lApp.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);
    lApp.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    KLocalizedString::setApplicationDomain("spectacle");

    KAboutData aboutData(QStringLiteral("spectacle"),
                         i18n("Spectacle"),
                         QStringLiteral(SPECTACLE_VERSION),
                         i18n("KDE Screenshot Utility"),
                         KAboutLicense::GPL_V2,
                         i18n("(C) 2015 Boudhayan Gupta"));
    aboutData.addAuthor(QStringLiteral("Boudhayan Gupta"), QString(), QStringLiteral("bgupta@kde.org"));
    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));
    KAboutData::setApplicationData(aboutData);
    lApp.setWindowIcon(QIcon::fromTheme(QStringLiteral("spectacle")));

    // set up the command line options parser

    QCommandLineParser lCmdLineParser;
    aboutData.setupCommandLine(&lCmdLineParser);

    lCmdLineParser.addOptions({
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
        {{QStringLiteral("c"), QStringLiteral("clipboard")},         i18n("In background mode, copy screenshot to clipboard")},
        {{QStringLiteral("w"), QStringLiteral("onclick")},           i18n("Wait for a click before taking screenshot. Invalidates delay")}
    });

    lCmdLineParser.process(lApp);
    aboutData.processCommandLine(&lCmdLineParser);

    // extract the capture mode

    Spectacle::CaptureMode lCaptureMode = Spectacle::CaptureMode::AllScreens;
    if (lCmdLineParser.isSet(QStringLiteral("current"))) {
        lCaptureMode = Spectacle::CaptureMode::CurrentScreen;
    } else if (lCmdLineParser.isSet(QStringLiteral("activewindow"))) {
        lCaptureMode = Spectacle::CaptureMode::ActiveWindow;
    } else if (lCmdLineParser.isSet(QStringLiteral("region"))) {
        lCaptureMode = Spectacle::CaptureMode::RectangularRegion;
    } else if (lCmdLineParser.isSet(QStringLiteral("windowundercursor"))) {
        lCaptureMode = Spectacle::CaptureMode::TransientWithParent;
    } else if (lCmdLineParser.isSet(QStringLiteral("transientonly"))) {
        lCaptureMode = Spectacle::CaptureMode::WindowUnderCursor;
    }

    // are we running in background or dbus mode?

    SpectacleCore::StartMode lStartMode = SpectacleCore::StartMode::Gui;
    bool lNotify = true;
    bool lCopyToClipboard = false;
    qint64 lDelayMsec = 0;
    QString lFileName = QString();

    if (lCmdLineParser.isSet(QStringLiteral("background"))) {
        lStartMode = SpectacleCore::StartMode::Background;
    } else if (lCmdLineParser.isSet(QStringLiteral("dbus"))) {
        lStartMode = SpectacleCore::StartMode::DBus;
    }

    switch(lStartMode) {
    case SpectacleCore::StartMode::Background:
        if (lCmdLineParser.isSet(QStringLiteral("nonotify"))) {
            lNotify = false;
        }

        if (lCmdLineParser.isSet(QStringLiteral("output"))) {
            lFileName = lCmdLineParser.value(QStringLiteral("output"));
        }

        if (lCmdLineParser.isSet(QStringLiteral("delay"))) {
            bool lParseOk = false;
            qint64 lDelayValue = lCmdLineParser.value(QStringLiteral("delay")).toLongLong(&lParseOk);
            if (lParseOk) {
                lDelayMsec = lDelayValue;
            }
        }

        if (lCmdLineParser.isSet(QStringLiteral("onclick"))) {
            lDelayMsec = -1;
        }

        if (lCmdLineParser.isSet(QStringLiteral("clipboard"))) {
                lCopyToClipboard = true;
        }

        lApp.setQuitOnLastWindowClosed(false);
        break;
    case SpectacleCore::StartMode::DBus:
        lApp.setQuitOnLastWindowClosed(false);
        break;
    case SpectacleCore::StartMode::Gui:
        break;
    }

    // release the kraken

    SpectacleCore lCore(lStartMode, lCaptureMode, lFileName, lDelayMsec, lNotify, lCopyToClipboard);
    QObject::connect(&lCore, &SpectacleCore::allDone, qApp, &QApplication::quit);

    // create the dbus connections

    new KDBusService(KDBusService::Multiple, &lCore);
    SpectacleDBusAdapter *lDBusAdapter = new SpectacleDBusAdapter(&lCore);
    QObject::connect(&lCore, &SpectacleCore::grabFailed, lDBusAdapter, &SpectacleDBusAdapter::ScreenshotFailed);
    QObject::connect(ExportManager::instance(), &ExportManager::imageSaved, [&](const QUrl &savedAt) {
        emit lDBusAdapter->ScreenshotTaken(savedAt.toLocalFile());
    });
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/"), &lCore);
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.Spectacle"));

    // fire it up

    return lApp.exec();
}
