/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ConfigUtils.h"
#include <KConfigGroup>
#include <KSharedConfig>

using namespace Qt::StringLiterals;

int main()
{
    // We only need to read spectaclerc, so we use SimpleConfig.
    auto spectaclerc = KSharedConfig::openConfig("spectaclerc"_L1, KConfig::SimpleConfig);

    // Remove old settings.
    spectaclerc->group(QStringLiteral("GuiConfig")).deleteEntry("videoFormat");
    auto saveGroup = spectaclerc->group(QStringLiteral("Save"));
    // These couldn't be changed via the GUI, but removing them anyway just in case
    saveGroup.deleteEntry("defaultVideoSaveLocation");
    saveGroup.deleteEntry("defaultSaveVideoFormat");
    saveGroup.deleteEntry("saveVideoFormat");

    // Copy to new groups and remove old groups
    auto imageSaveGroup = spectaclerc->group(QStringLiteral("ImageSave"));
    saveGroup.copyTo(&imageSaveGroup);
    saveGroup.deleteGroup();

    // Rename settings
    KeyMap oldNewMap{
        {"defaultSaveLocation", "imageSaveLocation"},
        {"compressionQuality", "imageCompressionQuality"},
        {"defaultSaveImageFormat", "preferredImageFormat"},
        {"saveFilenameFormat", "imageFilenameFormat"},
        {"lastSaveLocation", "lastImageSaveLocation"},
        {"lastSaveAsLocation", "lastImageSaveAsLocation"},
    };
    replaceEntryKeys(imageSaveGroup, oldNewMap);

    return spectaclerc->sync() ? 0 : 1;
}
