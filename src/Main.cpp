/*
 *  Copyright 2019 David Redondo <kde@david-redondo.de>
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
#include "settings.h"
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

    QApplication app(argc, argv);

    app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    KLocalizedString::setApplicationDomain("spectacle");
    QCoreApplication::setOrganizationDomain(QStringLiteral("org.kde"));

    KAboutData aboutData(QStringLiteral("spectacle"),
                         i18n("Spectacle"),
                         QStringLiteral(SPECTACLE_VERSION),
                         i18n("KDE Screenshot Utility"),
                         KAboutLicense::GPL_V2,
                         i18n("(C) 2015 Boudhayan Gupta"));
    aboutData.addAuthor(QStringLiteral("Boudhayan Gupta"), QString(), QStringLiteral("bgupta@kde.org"));
    aboutData.addAuthor(QStringLiteral("David Redondo"), QString(), QStringLiteral("kde@david-redondo.de"));
    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));
    KAboutData::setApplicationData(aboutData);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("spectacle")));

    SpectacleCore lCore;

    QCommandLineParser lCmdLineParser;
    aboutData.setupCommandLine(&lCmdLineParser);
    lCore.populateCommandLineParser(&lCmdLineParser);

    // first parsing for help-about
    lCmdLineParser.process(app.arguments());
    aboutData.processCommandLine(&lCmdLineParser);

    // and new-instance
    if (lCmdLineParser.isSet(QStringLiteral("new-instance"))){
        lCore.init();

        QObject::connect(&lCore, &SpectacleCore::allDone, &app, &QCoreApplication::quit, Qt::QueuedConnection);

        // fire it up
        lCore.onActivateRequested(app.arguments(), QStringLiteral());

        return app.exec();
    }

    // Ensure that we only launch a new instance if we need to
    // If there is already an instance running, we will quit here
    // and activateRequested signal is triggered
    // For some reason this does not work properly if behind an if
    KDBusService service(KDBusService::Unique, &lCore);

    // Delay initialisation after we now we are in the single instance or new-instance was passed, to avoid doing it each time spectacle executable is called
    lCore.init();

    // set up the KDBusService activateRequested slot
    QObject::connect(&service, &KDBusService::activateRequested, &lCore, &SpectacleCore::onActivateRequested);
    QObject::connect(&lCore, &SpectacleCore::allDone, &app, &QCoreApplication::quit, Qt::QueuedConnection);
    QObject::connect(qApp, &QApplication::aboutToQuit, Settings::self(), &Settings::save);

    // create the dbus connections
    SpectacleDBusAdapter *lDBusAdapter = new SpectacleDBusAdapter(&lCore);
    QObject::connect(&lCore, &SpectacleCore::grabFailed, lDBusAdapter, &SpectacleDBusAdapter::ScreenshotFailed);
    QObject::connect(ExportManager::instance(), &ExportManager::imageSaved, &lCore, [&](const QUrl &savedAt) {
        lDBusAdapter->ScreenshotTaken(savedAt.toLocalFile());
    });
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/"), &lCore);
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.Spectacle"));

    // fire it up
    lCore.onActivateRequested(app.arguments(), QStringLiteral());

    QObject::connect(&service, &KDBusService::activateActionRequested, &lCore, [&lCore] (const QString& actionName, const QVariant &parameters) {
        if (actionName == QLatin1String("FullScreenScreenShot")) {
            lCore.takeNewScreenshot(Spectacle::CaptureMode::AllScreens, 0, false, true);
        } else if (actionName == QLatin1String("CurrentMonitorScreenShot")) {
            lCore.takeNewScreenshot(Spectacle::CaptureMode::CurrentScreen, 0, false, true);
        } else if (actionName == QLatin1String("ActiveWindowScreenShot")) {
            lCore.takeNewScreenshot(Spectacle::CaptureMode::ActiveWindow, 0, false, true);
        } else if (actionName == QLatin1String("RectangularRegionScreenShot")) {
            lCore.takeNewScreenshot(Spectacle::CaptureMode::RectangularRegion, 0, false, true);
        }
    });

    return app.exec();
}
