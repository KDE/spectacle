/*
 *  SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "Config.h"
#include "ShortcutActions.h"
#include "SpectacleCore.h"
#include "CommandLineOptions.h"
#include "SpectacleDBusAdapter.h"
#include "ScreenShotEffect.h"
#include "settings.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDir>
#include <QSessionManager>

#include <KAboutData>
#include <KDBusService>
#include <KLocalizedString>
#include <KMessageBox>
#include <KWindowSystem>

int main(int argc, char **argv)
{
    // set up the application

    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
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

    // BUG: https://bugs.kde.org/show_bug.cgi?id=451842
    // We currently don't support desktop environments besides KDE Plasma on Wayland
    // because we have to rely on KWin's DBus API.
    if (KWindowSystem::isPlatformWayland() && !ScreenShotEffect::isLoaded()) {
        auto message = i18n("On Wayland, Spectacle requires KDE Plasma's KWin compositor, which does not seem to be available. Use Spectacle on KDE Plasma, or use a different screenshot tool.");
        qWarning().noquote() << message;
        if (commandLineParser.isSet(CommandLineOptions::self()->background)
            || commandLineParser.isSet(CommandLineOptions::self()->dbus)) {
            // Return early if not in GUI mode.
            return 1;
        } else {
            KMessageBox::error(nullptr, message);
        }
    }

    // Prevent session manager from restoring the app on start up.
    // https://bugs.kde.org/show_bug.cgi?id=430411
    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

    // If the new instance command line option has been specified,
    // use this alternative path for executing Spectacle.
    if (commandLineParser.isSet(CommandLineOptions::self()->newInstance)) {
        SpectacleCore spectacleCore;

        QObject::connect(qApp, &QApplication::aboutToQuit, Settings::self(), &Settings::save);
        QObject::connect(&spectacleCore, &SpectacleCore::allDone, &app, &QCoreApplication::quit, Qt::QueuedConnection);

        // fire it up
        spectacleCore.activate(app.arguments(), QDir::currentPath());

        return app.exec();
    }

    // With the StartupOption::Unique flag, this process will exit during the construction of
    // KDBusService if Spectacle has already been registered.
    // This object does not need a parent since it will be deleted when it falls out of scope.
    KDBusService service(KDBusService::Unique);

    SpectacleCore spectacleCore;

    QObject::connect(&service, &KDBusService::activateRequested, &spectacleCore, &SpectacleCore::activate);
    QObject::connect(&service, &KDBusService::activateActionRequested, &spectacleCore, [&spectacleCore](const QString &actionName) {
        if (actionName == ShortcutActions::self()->fullScreenAction()->objectName()) {
            spectacleCore.takeNewScreenshot(CaptureModeModel::AllScreens, 0);
        } else if (actionName == ShortcutActions::self()->currentScreenAction()->objectName()) {
            spectacleCore.takeNewScreenshot(CaptureModeModel::CurrentScreen, 0);
        } else if (actionName == ShortcutActions::self()->activeWindowAction()->objectName()) {
            spectacleCore.takeNewScreenshot(CaptureModeModel::ActiveWindow, 0);
        } else if (actionName == ShortcutActions::self()->windowUnderCursorAction()->objectName()) {
            spectacleCore.takeNewScreenshot(CaptureModeModel::WindowUnderCursor, 0);
        } else if (actionName == ShortcutActions::self()->regionAction()->objectName()) {
            spectacleCore.takeNewScreenshot(CaptureModeModel::RectangularRegion, 0);
        } else if (actionName == ShortcutActions::self()->openWithoutScreenshotAction()->objectName()) {
            spectacleCore.initGuiNoScreenshot();
        }
    });

    QObject::connect(&app, &QCoreApplication::aboutToQuit, Settings::self(), &Settings::save);
    QObject::connect(&spectacleCore, &SpectacleCore::allDone, &app, &QCoreApplication::quit, Qt::QueuedConnection);

    // create the dbus connections
    SpectacleDBusAdapter *dbusAdapter = new SpectacleDBusAdapter(&spectacleCore);
    QObject::connect(&spectacleCore, &SpectacleCore::grabFailed, dbusAdapter, &SpectacleDBusAdapter::ScreenshotFailed);
    QObject::connect(ExportManager::instance(), &ExportManager::imageExported,
                     &spectacleCore, [dbusAdapter](const ExportManager::Actions &actions, const QUrl &url) {
        Q_UNUSED(actions)
        Q_EMIT dbusAdapter->ScreenshotTaken(url.toLocalFile());
    });
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/"), &spectacleCore);
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.Spectacle"));

    // fire it up
    spectacleCore.activate(app.arguments(), QDir::currentPath());

    return app.exec();
}
