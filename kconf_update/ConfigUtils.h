/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <KConfigGroup>
#include <QDateTime>
#include <QFileInfo>
#include <QStandardPaths>

// automatically use this when including this header
using namespace Qt::StringLiterals;
using KeyMap = QMap<const char *, const char *>;
using ValueMap = QMap<QString, QString>;

inline void replaceEntryKeys(KConfigGroup &group, const KeyMap &oldNewMap)
{
    if (!group.exists()) {
        return;
    }
    for (auto it = oldNewMap.cbegin(); it != oldNewMap.cend(); ++it) {
        if (!group.hasKey(it.key())) {
            continue;
        }
        // Only write if new key is not empty
        if (!QByteArrayLiteral(it.value()).isEmpty()) {
            group.writeEntry(it.value(), group.readEntry(it.key()));
        }
        group.deleteEntry(it.key());
    }
};

inline void replaceEntryValues(KConfigGroup &group, const char *key,
                               const ValueMap &oldNewMap)
{
    if (!group.exists() || !group.hasKey(key)) {
        return;
    }
    for (auto it = oldNewMap.cbegin(); it != oldNewMap.cend(); ++it) {
        if (group.readEntry(key) != it.key()) {
            continue;
        }
        // Only write if new value is not empty
        if (!it.value().isEmpty()) {
            group.writeEntry(key, it.value());
        } else {
            // Delete if new value is empty because it'll be removed anyway.
            group.deleteEntry(key);
        }
    }
};

inline bool isFileOlderThanDateTime(const QString &fileName, const QString &isoDateTime = {})
{
    const auto path = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, fileName);
    // false if there is no existing user config.
    if (path.isEmpty() || !path.startsWith(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))) {
        return false;
    }

    // true if we aren't doing a datetime check
    if (isoDateTime.isEmpty()) {
        return true;
    }

    // false if the existing config is newer than the threshold datetime.
    QFileInfo fileInfo(path);
    auto configDateTime = fileInfo.birthTime();
    auto thresholdDateTime = QDateTime::fromString(isoDateTime, Qt::ISODate);
    if (!configDateTime.isValid() || !thresholdDateTime.isValid()
        || configDateTime > thresholdDateTime) {
        return false;
    }

    return true;
}

inline bool isEntryDefault(KConfigGroup &group, const char *key)
{
    return !group.exists() || group.readEntry(key).isEmpty();
}
