/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <KConfigGroup>
#include <KConfigSkeleton>

/**
 * A small collection of functions to help prevent duplicating the implementations of custom code used by the Settings class.
 */

/**
 * Gets a translated string, but the string is only translated once.
 * Mainly meant to keep file paths consistent.
 */
inline QString onceTranslatedString(KConfigSkeleton *kcs, const QString &groupName, const char *entryName, const QString &localizedDefault)
{
    auto config = kcs->sharedConfig();
    auto group = config->group(groupName);
    QString entry = group.readEntry(entryName);
    if (entry.isEmpty()) {
        entry = localizedDefault;
        group.writeEntry(entryName, entry);
        config->sync();
    }
    return entry;
}
