/*
 * SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ShortcutActions.h"

#include <QGuiApplication>

#include <KLocalizedString>

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
    // _launch
    {
        QAction *action = new QAction(i18n("Launch Spectacle"));
        action->setObjectName(QStringLiteral("_launch"));
        mActions.addAction(action->objectName(), action);
    }
    {
        QAction *action = new QAction(i18n("Capture Entire Desktop"));
        action->setObjectName(QStringLiteral("FullScreenScreenShot"));
        mActions.addAction(action->objectName(), action);
    }
    {
        QAction *action = new QAction(i18n("Capture Current Monitor"));
        action->setObjectName(QStringLiteral("CurrentMonitorScreenShot"));
        mActions.addAction(action->objectName(), action);
    }
    {
        QAction *action = new QAction(i18n("Capture Active Window"));
        action->setObjectName(QStringLiteral("ActiveWindowScreenShot"));
        mActions.addAction(action->objectName(), action);
    }
    {
        QAction *action = new QAction(i18n("Capture Rectangular Region"));
        action->setObjectName(QStringLiteral("RectangularRegionScreenShot"));
        mActions.addAction(action->objectName(), action);
    }
    {
        QAction *action = new QAction(i18n("Capture Window Under Cursor"));
        action->setObjectName(QStringLiteral("WindowUnderCursorScreenShot"));
        mActions.addAction(action->objectName(), action);
    }
}

KActionCollection *ShortcutActions::shortcutActions()
{
    return &mActions;
}

QString ShortcutActions::componentName() const
{
    return QGuiApplication::desktopFileName().append(QStringLiteral(".desktop"));
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
