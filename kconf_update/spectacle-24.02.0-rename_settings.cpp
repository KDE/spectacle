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

    auto general = spectaclerc->group("General");
    KeyMap generalOldNewMap{
        // Using a name that doesn't look like a signal handler.
        {"onLaunchAction", "launchAction"},
        // printKeyActionRunning looks like a bool
        {"printKeyActionRunning", "printKeyRunningAction"},
        // shorten name and make it consistent with selectionRect
        {"rememberLastRectangularRegion", "rememberSelectionRect"},
    };
    replaceEntryKeys(general, generalOldNewMap);
    // Fix spelling
    replaceEntryValues(general, "launchAction",
                       {{u"UseLastUsedCapturemode"_s, u"UseLastUsedCaptureMode"_s}});
    // Shorten enum values
    replaceEntryValues(general, "rememberSelectionRect",
                       {{u"UntilSpectacleIsClosed"_s, u"UntilClosed"_s}});

    auto guiConfig = spectaclerc->group("GuiConfig");
    KeyMap guiConfigOldNewMap{
        // More in line with naming elsewhere.
        {"cropRegion", "selectionRect"},
        // Using a name that doesn't look like a signal handler.
        {"onClickChecked", "captureOnClick"},
        // Using a consistent spelling for color in code.
        {"useLightMaskColour", "useLightMaskColor"},
    };
    replaceEntryKeys(guiConfig, guiConfigOldNewMap);

    auto imageSave = spectaclerc->group("ImageSave");
    replaceEntryKeys(imageSave, {{"imageFilenameFormat", "imageFilenameTemplate"}});

    auto videoSave = spectaclerc->group("VideoSave");
    replaceEntryKeys(videoSave, {{"videoFilenameFormat", "videoFilenameTemplate"}});

    return spectaclerc->sync() ? 0 : 1;
}
