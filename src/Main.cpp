/*
 *  SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "Config.h"
#include "SpectacleCore.h"
#include "CommandLineOptions.h"
#include "SpectacleDBusAdapter.h"
#include "settings.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QSessionManager>

#include <KAboutData>
#include <KDBusService>
#include <KLocalizedString>

int main(int argc, char **argv)
{
    // set up the application

    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    QApplication app(argc, argv);

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
    aboutData.addAuthor(QStringLiteral("Noah Davis"), QString(), QStringLiteral("noahadvs@gmail.com"));
    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));
    KAboutData::setApplicationData(aboutData);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("spectacle")));

    QCommandLineParser commandLineParser;
    aboutData.setupCommandLine(&commandLineParser);
    commandLineParser.addOptions(CommandLineOptions::self()->allOptions);

    // first parsing for help-about
    commandLineParser.process(app.arguments());
    aboutData.processCommandLine(&commandLineParser);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QGuiApplication::setFallbackSessionManagementEnabled(false);
#endif
    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

    SpectacleCore spectacleCore;

    // and new-instance
    if (commandLineParser.isSet(CommandLineOptions::self()->newInstance)) {
        spectacleCore.init();

        QObject::connect(qApp, &QApplication::aboutToQuit, Settings::self(), &Settings::save);
        QObject::connect(&spectacleCore, &SpectacleCore::allDone, &app, &QCoreApplication::quit, Qt::QueuedConnection);

        // fire it up
        spectacleCore.onActivateRequested(app.arguments(), QLatin1String());

        return app.exec();
    }

    // Ensure that we only launch a new instance if we need to
    // If there is already an instance running, we will quit here
    // and activateRequested signal is triggered
    // For some reason this does not work properly if behind an if
    KDBusService service(KDBusService::Unique, &lCore);

    // Delay initialisation after we now we are in the single instance or new-instance was passed, to avoid doing it each time spectacle executable is called
    spectacleCore.init();

    // set up the KDBusService activateRequested slot
    QObject::connect(&service, &KDBusService::activateRequested, &spectacleCore, &SpectacleCore::onActivateRequested);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, Settings::self(), &Settings::save);
    QObject::connect(&spectacleCore, &SpectacleCore::allDone, &app, &QCoreApplication::quit, Qt::QueuedConnection);

    // create the dbus connections
    SpectacleDBusAdapter *lDBusAdapter = new SpectacleDBusAdapter(&spectacleCore);
    QObject::connect(&spectacleCore, &SpectacleCore::grabFailed, lDBusAdapter, &SpectacleDBusAdapter::ScreenshotFailed);
    QObject::connect(ExportManager::instance(), &ExportManager::imageSaved, &spectacleCore, [&](const QUrl &savedAt) {
        Q_EMIT lDBusAdapter->ScreenshotTaken(savedAt.toLocalFile());
    });
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/"), &spectacleCore);
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.Spectacle"));

    // fire it up
    spectacleCore.onActivateRequested(app.arguments(), QLatin1String());

    return app.exec();
}
