/*
 * SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ShortcutActions.h"

#include <QGuiApplication>

#include <KLocalizedString>

using namespace Qt::StringLiterals;

ShortcutActions *ShortcutActions::self()
{
    static ShortcutActions self;
    return &self;
}

ShortcutActions::ShortcutActions()
    : mActions{nullptr, QString()}
{
    // everything here is named to match the jumplist actions in our .desktop file
    mActions.setComponentName(componentName());
    // qdbus org.kde.kglobalaccel /component/org_kde_spectacle_desktop org.kde.kglobalaccel.Component.shortcutNames
    // ActiveWindowScreenShot
    // WindowUnderCursorScreenShot
    // CurrentMonitorScreenShot
    // RectangularRegionScreenShot
    // FullScreenScreenShot
    // OpenWithoutScreenshot
    // RecordScreen
    // RecordWindow
    // RecordRegion
    // _launch
    {
        QAction *action = new QAction(i18nc("@action global shortcut", "Launch Spectacle"), &mActions);
        action->setObjectName(u"_launch"_s);
        action->setProperty("isConfigurationAction", true);
        mActions.addAction(action->objectName(), action);
    }
    {
        QAction *action = new QAction(i18nc("@action global shortcut", "Capture Entire Desktop"), &mActions);
        action->setObjectName(u"FullScreenScreenShot"_s);
        action->setProperty("isConfigurationAction", true);
        mActions.addAction(action->objectName(), action);
    }
    {
        QAction *action = new QAction(i18nc("@action global shortcut", "Capture Current Monitor"), &mActions);
        action->setObjectName(u"CurrentMonitorScreenShot"_s);
        action->setProperty("isConfigurationAction", true);
        mActions.addAction(action->objectName(), action);
    }
    {
        QAction *action = new QAction(i18nc("@action global shortcut", "Capture Active Window"), &mActions);
        action->setObjectName(u"ActiveWindowScreenShot"_s);
        action->setProperty("isConfigurationAction", true);
        mActions.addAction(action->objectName(), action);
    }
    {
        QAction *action = new QAction(i18nc("@action global shortcut", "Capture Rectangular Region"), &mActions);
        action->setObjectName(u"RectangularRegionScreenShot"_s);
        action->setProperty("isConfigurationAction", true);
        mActions.addAction(action->objectName(), action);
    }
    {
        auto wayland = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
        auto text = wayland ? i18nc("@action global shortcut", "Capture Selected Window") : i18nc("@action global shortcut", "Capture Window Under Cursor");
        QAction *action = new QAction(text, &mActions);
        action->setObjectName(u"WindowUnderCursorScreenShot"_s);
        action->setProperty("isConfigurationAction", true);
        mActions.addAction(action->objectName(), action);
    }
    {
        QAction *action = new QAction(i18nc("@action global shortcut", "Start/Stop Screen Recording"), &mActions);
        action->setObjectName(u"RecordScreen"_s);
        action->setProperty("isConfigurationAction", true);
        mActions.addAction(action->objectName(), action);
    }
    {
        QAction *action = new QAction(i18nc("@action global shortcut", "Start/Stop Window Recording"), &mActions);
        action->setObjectName(u"RecordWindow"_s);
        action->setProperty("isConfigurationAction", true);
        mActions.addAction(action->objectName(), action);
    }
    {
        QAction *action = new QAction(i18nc("@action global shortcut", "Start/Stop Region Recording"), &mActions);
        action->setObjectName(u"RecordRegion"_s);
        action->setProperty("isConfigurationAction", true);
        mActions.addAction(action->objectName(), action);
    }
    {
        QAction *action = new QAction(i18nc("@action global shortcut", "Launch Spectacle without capturing"), &mActions);
        action->setObjectName(u"OpenWithoutScreenshot"_s);
        action->setProperty("isConfigurationAction", true);
        mActions.addAction(action->objectName(), action);
    }
}

KActionCollection *ShortcutActions::shortcutActions()
{
    return &mActions;
}

QString ShortcutActions::componentName() const
{
    return QGuiApplication::desktopFileName().append(u".desktop"_s);
}

QAction *ShortcutActions::openAction() const
{
    return mActions.action(0);
}

QAction *ShortcutActions::fullScreenAction() const
{
    return mActions.action(1);
}

QAction *ShortcutActions::currentScreenAction() const
{
    return mActions.action(2);
}

QAction *ShortcutActions::activeWindowAction() const
{
    return mActions.action(3);
}

QAction *ShortcutActions::regionAction() const
{
    return mActions.action(4);
}

QAction *ShortcutActions::windowUnderCursorAction() const
{
    return mActions.action(5);
}

QAction *ShortcutActions::recordScreenAction() const
{
    return mActions.action(6);
}

QAction *ShortcutActions::recordWindowAction() const
{
    return mActions.action(7);
}

QAction *ShortcutActions::recordRegionAction() const
{
    return mActions.action(8);
}

QAction *ShortcutActions::openWithoutScreenshotAction() const
{
    return mActions.action(9);
}
