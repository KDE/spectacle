/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ScreenShotEffect.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QDBusReply>
#include <QDebug>

using namespace Qt::StringLiterals;

static const auto s_kwinService = u"org.kde.KWin"_s;
static const auto s_effectsObjectPath = u"/Effects"_s;
static const auto s_effectsInterface = u"org.kde.kwin.Effects"_s;

static const auto s_screenShot2Service = u"org.kde.KWin.ScreenShot2"_s;
static const auto s_screenShot2ObjectPath = u"/org/kde/KWin/ScreenShot2"_s;
static const auto s_screenShot2Interface = u"org.kde.KWin.ScreenShot2"_s;

static quint32 s_version = ScreenShotEffect::NullVersion;

bool ScreenShotEffect::isLoaded()
{
    return QDBusConnection::sessionBus().interface()->isServiceRegistered(s_screenShot2Service);
}

quint32 ScreenShotEffect::version()
{
    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(s_screenShot2Service)) {
        s_version = ScreenShotEffect::NullVersion;
    } else if (s_version == ScreenShotEffect::NullVersion) {
        QDBusInterface interface(s_screenShot2Service, s_screenShot2ObjectPath, s_screenShot2Interface);
        bool ok;
        auto version = interface.property("Version").toUInt(&ok);
        s_version = ok ? version : ScreenShotEffect::NullVersion;
    }
    return s_version;
}
