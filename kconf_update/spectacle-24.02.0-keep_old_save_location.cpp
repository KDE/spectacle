/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KConfigGroup>
#include <KSharedConfig>
#include <QFileInfo>
#include <QUrl>

using namespace Qt::StringLiterals;

int main()
{
    const auto fileName = "spectaclerc"_L1;
    const auto path = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, fileName);
    // Skip if there is no existing user config.
    if (!path.startsWith(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))) {
        return 0;
    }

    // Skip if the existing config is newer than when this script was committed to git master.
    auto configCreationTime = QFileInfo(path).birthTime();
    auto scriptCreationTime = QDateTime::fromString("2023-10-11"_L1, Qt::ISODate);
    if (!configCreationTime.isValid() || !scriptCreationTime.isValid()
        || configCreationTime > scriptCreationTime) {
        return 0;
    }

    // We only need to read spectaclerc, so we use SimpleConfig.
    auto spectaclerc = KSharedConfig::openConfig(fileName, KConfig::SimpleConfig);

    // Preserve old defaults for existing users that didn't already have these set.
    auto imageSaveGroup = spectaclerc->group(QStringLiteral("ImageSave"));
    if (!imageSaveGroup.exists() || imageSaveGroup.readEntry("imageSaveLocation").isEmpty()) {
        const auto url = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + u'/');
        imageSaveGroup.writeEntry("imageSaveLocation", url);
    }

    auto videoSaveGroup = spectaclerc->group(QStringLiteral("VideoSave"));
    if (!videoSaveGroup.exists() || videoSaveGroup.readEntry("videoSaveLocation").isEmpty()) {
        const auto url = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) + u'/');
        videoSaveGroup.writeEntry("videoSaveLocation", url);
    }

    return spectaclerc->sync() ? 0 : 1;
}
