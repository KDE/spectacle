/*
 *  SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "Config.h"
#include "SpectacleCommon.h"
#include "SpectacleCore.h"
#include "SpectacleDBusAdapter.h"
#include "settings.h"

#ifdef KIMAGEANNOTATOR_CAN_LOAD_TRANSLATIONS
#include <kImageAnnotator/KImageAnnotator.h>
#endif

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
    if (lCmdLineParser.isSet(QStringLiteral("new-instance"))) {
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

#ifdef KIMAGEANNOTATOR_CAN_LOAD_TRANSLATIONS
    kImageAnnotator::loadTranslations();
#endif

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

    return app.exec();
}
