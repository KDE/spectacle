/*
 * Copyright (C) 2019  David Redondo <kde@david-redondo.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "ShortcutActions.h"

#include <QGuiApplication>

#include <KLocalizedString>

ShortcutActions* ShortcutActions::self()
{
    static ShortcutActions self;
    return &self;
}

ShortcutActions::ShortcutActions() : mActions{nullptr, QString()}
{
    //everything here is named to match the jumplist actions in our .desktop file
    mActions.setComponentName(componentName());
    //qdbus org.kde.kglobalaccel /component/org_kde_spectacle_desktop org.kde.kglobalaccel.Component.shortcutNames
    // ActiveWindowScreenShot
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
}

KActionCollection* ShortcutActions::shortcutActions()
{
    return &mActions;
}

QString ShortcutActions::componentName() const
{
    return QGuiApplication::desktopFileName().append(QStringLiteral(".desktop"));
}

QAction* ShortcutActions::openAction() const
{
    return mActions.action(0);
}

QAction* ShortcutActions::fullScreenAction() const
{
    return mActions.action(1);
}

QAction* ShortcutActions::currentScreenAction() const
{
    return mActions.action(2);
}

QAction* ShortcutActions::activeWindowAction() const
{
    return mActions.action(3);
}

QAction* ShortcutActions::regionAction() const
{
    return mActions.action(4);
}
