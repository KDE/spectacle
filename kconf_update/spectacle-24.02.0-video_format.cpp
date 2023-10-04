/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KConfigGroup>
#include <KSharedConfig>
#include <kconfiggroup.h>

using namespace Qt::StringLiterals;

inline void renameEntry(KConfigGroup *group, const char *readFrom, const char *writeTo) {
    if (!group || !group->exists() || !group->hasKey(readFrom)) {
        return;
    }
    group->writeEntry(writeTo, group->readEntry(readFrom));
    group->deleteEntry(readFrom);
};

int main()
{
    // We only need to read spectaclerc, so we use SimpleConfig.
    auto spectaclerc = KSharedConfig::openConfig("spectaclerc"_L1, KConfig::SimpleConfig);

    // Remove old settings.
    spectaclerc->group("GuiConfig").deleteEntry("videoFormat");
    auto saveGroup = spectaclerc->group("Save");
    // These couldn't be changed via the GUI, but removing them anyway just in case
    saveGroup.deleteEntry("defaultVideoSaveLocation");
    saveGroup.deleteEntry("defaultSaveVideoFormat");
    saveGroup.deleteEntry("saveVideoFormat");

    // Copy to new groups and remove old groups
    auto imageSaveGroup = spectaclerc->group("ImageSave");
    saveGroup.copyTo(&imageSaveGroup);
    saveGroup.deleteGroup();

    // Rename settings
    renameEntry(&imageSaveGroup, "defaultSaveLocation", "imageSaveLocation");
    renameEntry(&imageSaveGroup, "compressionQuality", "imageCompressionQuality");
    renameEntry(&imageSaveGroup, "defaultSaveImageFormat", "preferredImageFormat");
    renameEntry(&imageSaveGroup, "saveFilenameFormat", "imageFilenameFormat");
    renameEntry(&imageSaveGroup, "lastSaveLocation", "lastImageSaveLocation");
    renameEntry(&imageSaveGroup, "lastSaveAsLocation", "lastImageSaveAsLocation");

    return spectaclerc->sync() ? 0 : 1;
}
