/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PlasmaVersion.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QDebug>

static quint32 s_plasmaVersion = 0;
static const auto s_plasmashellService = QStringLiteral("org.kde.plasmashell");

quint32 PlasmaVersion::get()
{
    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(s_plasmashellService)) {
        s_plasmaVersion = 0;
    } else if (s_plasmaVersion == 0) {
        auto message = QDBusMessage::createMethodCall(s_plasmashellService,
                                                      QStringLiteral("/MainApplication"),
                                                      QStringLiteral("org.freedesktop.DBus.Properties"),
                                                      QStringLiteral("Get"));

        message.setArguments({QStringLiteral("org.qtproject.Qt.QCoreApplication"), QStringLiteral("applicationVersion")});

        const auto resultMessage = QDBusConnection::sessionBus().call(message);
        if (resultMessage.type() != QDBusMessage::ReplyMessage) {
            qWarning() << "Error querying plasma version" << resultMessage.errorName() << resultMessage.errorMessage();
            return s_plasmaVersion;
        }
        QDBusVariant val = resultMessage.arguments().at(0).value<QDBusVariant>();

        const QString rawVersion = val.variant().value<QString>();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        const QVector<QStringRef> splitted = rawVersion.splitRef(QLatin1Char('.'));
#else
        const QVector<QStringView> splitted = QStringView(rawVersion).split(QLatin1Char('.'));
#endif
        if (splitted.size() != 3) {
            qWarning() << "error parsing plasma version";
            return s_plasmaVersion;
        }
        bool ok;
        auto major = splitted[0].toShort(&ok);
        if (!ok) {
            qWarning() << "error parsing plasma major version";
            return s_plasmaVersion;
        }
        auto minor = splitted[1].toShort(&ok);
        if (!ok) {
            qWarning() << "error parsing plasma minor version";
            return s_plasmaVersion;
        }
        auto patch = splitted[2].toShort(&ok);
        if (!ok) {
            qWarning() << "error parsing plasma patch version";
            return s_plasmaVersion;
        }
        s_plasmaVersion = check(major, minor, patch);
    }
    return s_plasmaVersion;
}

quint32 PlasmaVersion::check(quint8 major, quint8 minor, quint8 patch)
{
    return (major << 16) | (minor << 8) | patch;
}
