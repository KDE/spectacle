#include <iostream>

#include <KActionCollection>
#include <KConfig>
#include <KConfigGroup>
#include <KGlobalAccel>
#include <KLocalizedString>
#include <QCoreApplication>
#include <QDBusInterface>
#include <QString>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QDBusInterface khotkeys(QStringLiteral("org.kde.kded5"), QStringLiteral("/modules/khotkeys"), QStringLiteral("org.kde.khotkeys"));
    KConfig khotkeysrc(QStringLiteral("khotkeysrc"), KConfig::SimpleConfig);
    int dataCount = KConfigGroup(&khotkeysrc, "Data").readEntry("DataCount", 0);
    bool found_spectacle = false;
    int spectacleIndex;
    for (int i = 1; i <= dataCount; ++i) {
        if (KConfigGroup(&khotkeysrc, QStringLiteral("Data_%1").arg(i)).readEntry("ImportId", QString()) == QLatin1String("spectacle")) {
            found_spectacle = true;
            spectacleIndex = i;
            break;
        }
    }
    QList<QKeySequence> launchKey;
    QList<QKeySequence> fullScreenKey;
    QList<QKeySequence> regionKey;
    QList<QKeySequence> activeWindowKey;
    QStringList ids;
    if (found_spectacle) {
        for (int i = 1; i <= 4; ++i) {
            QString groupName = QStringLiteral("Data_%1_%2").arg(spectacleIndex).arg(i);
            QString method = KConfigGroup(&khotkeysrc, groupName + QStringLiteral("Actions0")).readEntry("Call");
            QString id = KConfigGroup(&khotkeysrc, groupName + QStringLiteral("Triggers0")).readEntry("Uuid");
            QList<QKeySequence> shortcut = KGlobalAccel::self()->globalShortcut(QStringLiteral("khotkeys"), id);
            ids.append(id);
            /* Name and Comment field are translated but we can find out which action is which by looking at the called
             *  D-Bus Method */
            if (method == QLatin1String("StartAgent")) {
                launchKey = shortcut;
            } else if (method == QLatin1String("FullScreen")) {
                fullScreenKey = shortcut;
            } else if (method == QLatin1String("ActiveWindow")) {
                activeWindowKey = shortcut;
            } else if (method == QLatin1String("RectangularRegion")) {
                regionKey = shortcut;
            }
            // Delete the groups from khotkeysrc
            khotkeysrc.deleteGroup(groupName);
            khotkeysrc.deleteGroup(groupName + QStringLiteral("Actions"));
            khotkeysrc.deleteGroup(groupName + QStringLiteral("Actions0"));
            khotkeysrc.deleteGroup(groupName + QStringLiteral("Conditions"));
            khotkeysrc.deleteGroup(groupName + QStringLiteral("Triggers"));
            khotkeysrc.deleteGroup(groupName + QStringLiteral("Triggers0"));
        }
        khotkeysrc.deleteGroup(QStringLiteral("Data_%1").arg(spectacleIndex));
        khotkeysrc.deleteGroup(QStringLiteral("Data_%1Conditions").arg(spectacleIndex));
        khotkeysrc.sync();
    }
    QDBusInterface kglobalaccel(QStringLiteral("org.kde.kglobalaccel"), QStringLiteral("/kglobalaccel"), QStringLiteral("org.kde.KGlobalAccel"));
    // Unregister the khotkeysActions from globalAccel, removeAll didn't Work, so using D-Bus
    for (const QString &action : ids) {
        kglobalaccel.call(QStringLiteral("unregister"), QStringLiteral("khotkeys"), action);
    }
    // Setup the default Shortcuts
    KActionCollection shortCutActions(static_cast<QObject *>(nullptr));
    shortCutActions.setComponentName(QStringLiteral("org.kde.spectacle.desktop"));
    shortCutActions.setComponentDisplayName(QStringLiteral("Spectacle"));
    {
        QAction *action = new QAction(i18n("Launch Spectacle"));
        action->setObjectName(QStringLiteral("_launch"));
        shortCutActions.addAction(action->objectName(), action);
    }
    {
        QAction *action = new QAction(i18n("Capture Entire Desktop"));
        action->setObjectName(QStringLiteral("FullScreenScreenShot"));
        shortCutActions.addAction(action->objectName(), action);
    }
    {
        QAction *action = new QAction(i18n("Capture Current Monitor"));
        action->setObjectName(QStringLiteral("CurrentMonitorScreenShot"));
        shortCutActions.addAction(action->objectName(), action);
    }
    {
        QAction *action = new QAction(i18n("Capture Active Window"));
        action->setObjectName(QStringLiteral("ActiveWindowScreenShot"));
        shortCutActions.addAction(action->objectName(), action);
    }
    {
        QAction *action = new QAction(i18n("Capture Rectangular Region"));
        action->setObjectName(QStringLiteral("RectangularRegionScreenShot"));
        shortCutActions.addAction(action->objectName(), action);
    }
    QAction *openAction = shortCutActions.action(QStringLiteral("_launch"));
    KGlobalAccel::self()->setDefaultShortcut(openAction, {Qt::Key_Print});
    QAction *fullScreenAction = shortCutActions.action(QStringLiteral("FullScreenScreenShot"));
    KGlobalAccel::self()->setDefaultShortcut(fullScreenAction, {Qt::SHIFT | Qt::Key_Print});
    // QAction* currentScreenAction = shortCutActions.action(QStringLiteral("CurrentMonitorScreenShot"));
    QAction *activeWindowAction = shortCutActions.action(QStringLiteral("ActiveWindowScreenShot"));
    KGlobalAccel::self()->setDefaultShortcut(activeWindowAction, {Qt::META | Qt::Key_Print});
    QAction *regionAction = shortCutActions.action(QStringLiteral("RectangularRegionScreenShot"));
    KGlobalAccel::self()->setDefaultShortcut(regionAction, {Qt::META | Qt::SHIFT | Qt::Key_Print});
    // Finally reinstate the old shortcuts
    if (found_spectacle) {
        KGlobalAccel::self()->setShortcut(openAction, launchKey, KGlobalAccel::NoAutoloading);
        KGlobalAccel::self()->setShortcut(fullScreenAction, fullScreenKey, KGlobalAccel::NoAutoloading);
        KGlobalAccel::self()->setShortcut(activeWindowAction, activeWindowKey, KGlobalAccel::NoAutoloading);
        KGlobalAccel::self()->setShortcut(regionAction, regionKey, KGlobalAccel::NoAutoloading);
    }
}
