/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ConfigUtils.h"
#include <KConfigGroup>
#include <KSharedConfig>

using namespace Qt::StringLiterals;

int main()
{
    const auto fileName = u"spectaclerc"_s;
    if (!isFileOlderThanDateTime(fileName, u"2024-02-28T00:00:00Z"_s)) {
        return 0;
    }

    // We only need to read spectaclerc, so we use SimpleConfig.
    auto spectaclerc = KSharedConfig::openConfig(fileName, KConfig::SimpleConfig);

    // Preserve old defaults for existing users that didn't already have these set.
    auto imageSaveGroup = spectaclerc->group(QStringLiteral("ImageSave"));
    if (isEntryDefault(imageSaveGroup, "imageFilenameTemplate")) {
        imageSaveGroup.writeEntry("imageFilenameTemplate", "Screenshot_<yyyy><MM><dd>_<HH><mm><ss>");
    }

    auto videoSaveGroup = spectaclerc->group(QStringLiteral("VideoSave"));
    if (isEntryDefault(videoSaveGroup, "videoFilenameTemplate")) {
        videoSaveGroup.writeEntry("videoFilenameTemplate", "Screencast_<yyyy><MM><dd>_<HH><mm><ss>");
    }

    return spectaclerc->sync() ? 0 : 1;
}
