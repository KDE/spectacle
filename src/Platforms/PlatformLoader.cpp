/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PlatformLoader.h"
#include "Config.h"
#include "PlasmaVersion.h"
#include "ScreenShotEffect.h"

#include <KLocalizedString>
#include <KWindowSystem>

#include <QCoreApplication>
#include <QDebug>
#include <QPluginLoader>

#include <filesystem>

#include "spectacle_debug.h"

using namespace Qt::StringLiterals;

template<typename T>
T *loadPlatform(const QString &name)
{
    const auto pluginDirs = QCoreApplication::libraryPaths();
    for (const auto &dir : pluginDirs) {
        const auto path = std::filesystem::path(dir.toStdString()) / "spectacle";

        if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
            continue;
        }

        for (const auto &entry : std::filesystem::directory_iterator(path)) {
            QPluginLoader loader(QString::fromStdString(entry.path()));
            const auto metaData = loader.metaData().value(u"MetaData").toObject();
            if (metaData.value("platform").toString() == name) {
                return qobject_cast<T *>(loader.instance());
            }
        }
    }

    return nullptr;
}

ImagePlatformPtr loadImagePlatform()
{
    auto platformName = qgetenv("SPECTACLE_IMAGE_PLATFORM");
    if (platformName.isEmpty()) {
        // Check XDG_SESSION_TYPE because Spectacle might be using the XCB platform via XWayland
        const bool isReallyX11 = KWindowSystem::isPlatformX11() && qstrcmp(qgetenv("XDG_SESSION_TYPE").constData(), "wayland") != 0;
        if (ScreenShotEffect::isLoaded() && ScreenShotEffect::version() != ScreenShotEffect::NullVersion
            && (!isReallyX11 || PlasmaVersion::get() >= PlasmaVersion::check(5, 27, 8))) {
            platformName = "image_kwin";
        } else if (isReallyX11) {
            platformName = "image_xcb";
        }
    }

    auto object = loadPlatform<ImagePlatform>(platformName);
    if (object) {
        qCInfo(SPECTACLE_LOG) << "Loaded image platform" << platformName;
        return ImagePlatformPtr(object);
    } else {
        qCWarning(SPECTACLE_LOG) << "Could not load image platform" << platformName;
    }

    auto nullPlatform = loadPlatform<ImagePlatform>(u"image_null"_s);
    if (!nullPlatform) {
        qCFatal(SPECTACLE_LOG) << "Failed to load platform" << platformName << "and failed to load null platform fallback!";
    }

    return ImagePlatformPtr(nullPlatform);
}

VideoPlatformPtr loadVideoPlatform()
{
    auto platformName = qgetenv("SPECTACLE_VIDEO_PLATFORM");
    if (platformName.isEmpty()) {
        if (KWindowSystem::isPlatformWayland()) {
            platformName = "video_wayland";
        }
    }

    if (KWindowSystem::isPlatformX11()) {
        qCWarning(SPECTACLE_LOG) << "Screen recording is not availabe on X11";
        platformName = "video_null";
    }

    auto object = loadPlatform<VideoPlatform>(platformName);
    if (object) {
        qCInfo(SPECTACLE_LOG) << "Loaded video platform" << platformName;
        return VideoPlatformPtr(object);
    } else {
        qCWarning(SPECTACLE_LOG) << "Could not load video platform" << platformName;
    }

    auto nullPlatform = loadPlatform<VideoPlatform>(u"video_null"_s);
    if (!nullPlatform) {
        qCFatal(SPECTACLE_LOG) << "Failed to load platform" << platformName << "and failed to load null platform fallback!";
    }
    return VideoPlatformPtr(nullPlatform);
}
