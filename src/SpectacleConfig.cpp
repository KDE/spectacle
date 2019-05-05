/*
 *  Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "SpectacleConfig.h"

#include <KWindowSystem>

SpectacleConfig::SpectacleConfig(QObject *parent) :
    QObject(parent)
{
    mConfig = KSharedConfig::openConfig(QStringLiteral("spectaclerc"));
    mGeneralConfig = KConfigGroup(mConfig, "General");
    mGuiConfig = KConfigGroup(mConfig, "GuiConfig");
}

SpectacleConfig::~SpectacleConfig()
{}

SpectacleConfig* SpectacleConfig::instance()
{
    static SpectacleConfig instance;
    return &instance;
}

QString SpectacleConfig::defaultFilename() const
{
    return QStringLiteral("Screenshot");
}

QString SpectacleConfig::defaultTimestampTemplate() const
{
    // includes separator at the front
    return QStringLiteral("_%Y%M%D_%H%m%S");
}

// lastSaveAsLocation

QUrl SpectacleConfig::lastSaveAsFile() const
{
    return mGeneralConfig.readEntry(QStringLiteral("lastSaveAsFile"),
                                    QUrl(this->defaultSaveLocation()));
}

void SpectacleConfig::setLastSaveAsFile(const QUrl &location)
{
    mGeneralConfig.writeEntry(QStringLiteral("lastSaveAsFile"), location);
    mGeneralConfig.sync();
}

QUrl SpectacleConfig::lastSaveAsLocation() const
{
    return this->lastSaveAsFile().adjusted(QUrl::RemoveFilename);
}

// lastSaveLocation

QUrl SpectacleConfig::lastSaveFile() const
{
    return mGeneralConfig.readEntry(QStringLiteral("lastSaveFile"),
                                    QUrl(this->defaultSaveLocation()));
}

void SpectacleConfig::setLastSaveFile(const QUrl &location)
{
    mGeneralConfig.writeEntry(QStringLiteral("lastSaveFile"), location);
    mGeneralConfig.sync();
}

QUrl SpectacleConfig::lastSaveLocation() const 
{
    return this->lastSaveFile().adjusted(QUrl::RemoveFilename);
}

// cropRegion

QRect SpectacleConfig::cropRegion() const
{
    return mGuiConfig.readEntry(QStringLiteral("cropRegion"), QRect());
}

void SpectacleConfig::setCropRegion(const QRect &region)
{
    mGuiConfig.writeEntry(QStringLiteral("cropRegion"), region);
    mGuiConfig.sync();
}

// onclick

bool SpectacleConfig::onClickChecked() const
{
    return mGuiConfig.readEntry(QStringLiteral("onClickChecked"), false);
}

void SpectacleConfig::setOnClickChecked(bool enabled)
{
    mGuiConfig.writeEntry(QStringLiteral("onClickChecked"), enabled);
    mGuiConfig.sync();
}

// include pointer

bool SpectacleConfig::includePointerChecked() const
{
    return mGuiConfig.readEntry(QStringLiteral("includePointer"), true);
}

void SpectacleConfig::setIncludePointerChecked(bool enabled)
{
    mGuiConfig.writeEntry(QStringLiteral("includePointer"), enabled);
    mGuiConfig.sync();
}

// include decorations

bool SpectacleConfig::includeDecorationsChecked() const
{
    return mGuiConfig.readEntry(QStringLiteral("includeDecorations"), true);
}

void SpectacleConfig::setIncludeDecorationsChecked(bool enabled)
{
    mGuiConfig.writeEntry(QStringLiteral("includeDecorations"), enabled);
    mGuiConfig.sync();
}

// capture transient window only

bool SpectacleConfig::captureTransientWindowOnlyChecked() const
{
    return mGuiConfig.readEntry(QStringLiteral("transientOnly"), false);
}

void SpectacleConfig::setCaptureTransientWindowOnlyChecked(bool enabled)
{
    mGuiConfig.writeEntry(QStringLiteral("transientOnly"), enabled);
    mGuiConfig.sync();
}

// quit after saving, copying, or exporting the image

bool SpectacleConfig::quitAfterSaveOrCopyChecked() const
{
    return mGuiConfig.readEntry(QStringLiteral("quitAfterSaveCopyExport"), false);
}

void SpectacleConfig::setQuitAfterSaveOrCopyChecked(bool enabled)
{
    mGuiConfig.writeEntry(QStringLiteral("quitAfterSaveCopyExport"), enabled);
    mGuiConfig.sync();
}

// show magnifier

bool SpectacleConfig::showMagnifierChecked() const
{
    return mGuiConfig.readEntry(QStringLiteral("showMagnifier"), false);
}

void SpectacleConfig::setShowMagnifierChecked(bool enabled)
{
    mGuiConfig.writeEntry(QStringLiteral("showMagnifier"), enabled);
    mGuiConfig.sync();
}

// release mouse-button to capture

bool SpectacleConfig::useReleaseToCapture() const
{
    return mGuiConfig.readEntry(QStringLiteral("useReleaseToCapture"), false);
}

void SpectacleConfig::setUseReleaseToCaptureChecked(bool enabled)
{
    mGuiConfig.writeEntry(QStringLiteral("useReleaseToCapture"), enabled);
    mGuiConfig.sync();
}

// capture delay

qreal SpectacleConfig::captureDelay() const
{
    return mGuiConfig.readEntry(QStringLiteral("captureDelay"), 0.0);
}

void SpectacleConfig::setCaptureDelay(qreal delay)
{
    mGuiConfig.writeEntry(QStringLiteral("captureDelay"), delay);
    mGuiConfig.sync();
}

// capture mode

int SpectacleConfig::captureMode() const
{
    return std::max(0, mGuiConfig.readEntry(QStringLiteral("captureModeIndex"), 0));
}

void SpectacleConfig::setCaptureMode(int index)
{
    mGuiConfig.writeEntry(QStringLiteral("captureModeIndex"), index);
    mGuiConfig.sync();
}

// remember last rectangular region

bool SpectacleConfig::rememberLastRectangularRegion() const
{
    return mGuiConfig.readEntry(QStringLiteral("rememberLastRectangularRegion"), true);
}

void SpectacleConfig::setRememberLastRectangularRegion(bool enabled)
{
    mGuiConfig.writeEntry(QStringLiteral("rememberLastRectangularRegion"), enabled);
    mGuiConfig.sync();
}

bool SpectacleConfig::alwaysRememberRegion() const
{
    // Default Value is for compatibility reasons as the old behavior was always to remember across restarts
    bool useOldBehavior = mGuiConfig.readEntry(QStringLiteral("rememberLastRectangularRegion"), false);
    return mGuiConfig.readEntry(QStringLiteral("alwaysRememberRegion"), useOldBehavior);
}

void SpectacleConfig::setAlwaysRememberRegion (bool enabled)
{
    mGuiConfig.writeEntry(QStringLiteral("alwaysRememberRegion"), enabled);
    mGuiConfig.sync();
}

// use light region mask colour

bool SpectacleConfig::useLightRegionMaskColour() const
{
    return mGuiConfig.readEntry(QStringLiteral("useLightMaskColour"), false);
}

void SpectacleConfig::setUseLightRegionMaskColour(bool enabled)
{
    mGuiConfig.writeEntry(QStringLiteral("useLightMaskColour"), enabled);
    mGuiConfig.sync();
}

// compression quality setting

int SpectacleConfig::compressionQuality() const
{
    return mGuiConfig.readEntry(QStringLiteral("compressionQuality"), 90);
}

void SpectacleConfig::setCompressionQuality(int value)
{
    mGuiConfig.writeEntry(QStringLiteral("compressionQuality"), value);
    mGuiConfig.sync();
}

// last used save mode

SaveMode SpectacleConfig::lastUsedSaveMode() const
{
    switch (mGuiConfig.readEntry(QStringLiteral("lastUsedSaveMode"), 0)) {
        default:
        case 0:
            return SaveMode::SaveAs;
        case 1:
            return SaveMode::Save;
    }
}

void SpectacleConfig::setLastUsedSaveMode(SaveMode mode)
{
    mGuiConfig.writeEntry(QStringLiteral("lastUsedSaveMode"), static_cast<int>(mode));
    mGuiConfig.sync();
}

// autosave filename format

QString SpectacleConfig::autoSaveFilenameFormat() const
{
    const QString sff = mGeneralConfig.readEntry(QStringLiteral("save-filename-format"),
                                           QString(defaultFilename() + defaultTimestampTemplate()));
    return sff.isEmpty() ? QStringLiteral("%d") : sff;
}

void SpectacleConfig::setAutoSaveFilenameFormat(const QString &format)
{
    mGeneralConfig.writeEntry(QStringLiteral("save-filename-format"), format);
    mGeneralConfig.sync();
}

// autosave location

QString SpectacleConfig::defaultSaveLocation() const
{
    return mGeneralConfig.readPathEntry(QStringLiteral("default-save-location"),
                          QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
}

void SpectacleConfig::setDefaultSaveLocation(const QString &location)
{
    mGeneralConfig.writePathEntry(QStringLiteral("default-save-location"), location);
    mGeneralConfig.sync();
}

// copy file location to clipboard after saving

bool SpectacleConfig::copySaveLocationToClipboard() const
{
    return mGeneralConfig.readEntry(QStringLiteral("copySaveLocation"), false);
}

void SpectacleConfig::setCopySaveLocationToClipboard(bool enabled)
{
    mGeneralConfig.writeEntry(QStringLiteral("copySaveLocation"), enabled);
    mGeneralConfig.sync();
}

// autosave image format

QString SpectacleConfig::saveImageFormat() const
{
    return mGeneralConfig.readEntry(QStringLiteral("default-save-image-format"),
                             QStringLiteral("png"));
}

void SpectacleConfig::setSaveImageFormat(const QString &saveFmt)
{
    mGeneralConfig.writeEntry(QStringLiteral("default-save-image-format"), saveFmt);
    mGeneralConfig.sync();
}

SpectacleConfig::PrintKeyActionRunning SpectacleConfig::printKeyActionRunning() const
{
    mConfig->reparseConfiguration();
    int newScreenshotAction = static_cast<int>(SpectacleConfig::PrintKeyActionRunning::TakeNewScreenshot);
    int readValue = mGuiConfig.readEntry(QStringLiteral("printKeyActionRunning"), newScreenshotAction);
    if ((KWindowSystem::isPlatformWayland() || qstrcmp(qgetenv("XDG_SESSION_TYPE"), "wayland") == 0 )
        && readValue == SpectacleConfig::PrintKeyActionRunning::FocusWindow)  {
        return SpectacleConfig::PrintKeyActionRunning::TakeNewScreenshot;
    }
    return static_cast<SpectacleConfig::PrintKeyActionRunning>(readValue);
}

void SpectacleConfig::setPrintKeyActionRunning (SpectacleConfig::PrintKeyActionRunning action)
{
    mGuiConfig.writeEntry(QStringLiteral("printKeyActionRunning"), static_cast<int>(action));
    mGuiConfig.sync();
}
