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

#include <QStandardPaths>

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

// lastSaveAsLocation

QUrl SpectacleConfig::lastSaveAsLocation() const
{
    return mGeneralConfig.readEntry(QStringLiteral("lastSaveAsLocation"),
                                    QUrl::fromUserInput(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)));
}

void SpectacleConfig::setLastSaveAsLocation(const QUrl &location)
{
    mGeneralConfig.writeEntry(QStringLiteral("lastSaveAsLocation"), location);
    mGeneralConfig.sync();
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
    return mGuiConfig.readEntry(QStringLiteral("captureModeIndex"), 0);
}

void SpectacleConfig::setCaptureMode(int index)
{
    mGuiConfig.writeEntry(QStringLiteral("captureModeIndex"), index);
    mGuiConfig.sync();
}

// dynamic save button

bool SpectacleConfig::useDynamicSaveButton() const
{
    return mGuiConfig.readEntry(QStringLiteral("dynamicSaveButton"), true);
}

void SpectacleConfig::setUseDynamicSaveButton(bool enabled)
{
    mGuiConfig.writeEntry(QStringLiteral("dynamicSaveButton"), enabled);
    mGuiConfig.sync();
}

// remember last rectangular region

bool SpectacleConfig::rememberLastRectangularRegion() const
{
    return mGuiConfig.readEntry(QStringLiteral("rememberLastRectangularRegion"), false);
}

void SpectacleConfig::setRememberLastRectangularRegion(bool enabled)
{
    mGuiConfig.writeEntry(QStringLiteral("rememberLastRectangularRegion"), enabled);
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

// last used save mode

SaveMode SpectacleConfig::lastUsedSaveMode() const
{
    return static_cast<SaveMode>(mGuiConfig.readEntry(QStringLiteral("lastUsedSaveMode"), 0));
}

void SpectacleConfig::setLastUsedSaveMode(SaveMode mode)
{
    mGuiConfig.writeEntry(QStringLiteral("lastUsedSaveMode"), static_cast<int>(mode));
    mGuiConfig.sync();
}

// autosave filename format

QString SpectacleConfig::autoSaveFilenameFormat() const
{
    return mGeneralConfig.readEntry(QStringLiteral("save-filename-format"),
                          QStringLiteral("Screenshot_%Y%M%D_%H%m%S"));
}

void SpectacleConfig::setAutoSaveFilenameFormat(const QString &format)
{
    mGeneralConfig.writeEntry(QStringLiteral("save-filename-format"), format);
    mGeneralConfig.sync();
}

// autosave location

QString SpectacleConfig::autoSaveLocation() const
{
    return mGeneralConfig.readPathEntry(QStringLiteral("default-save-location"),
                          QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
}

void SpectacleConfig::setAutoSaveLocation(const QString &location)
{
    mGeneralConfig.writePathEntry(QStringLiteral("default-save-location"), location);
    mGeneralConfig.sync();
}

// copy save location to clipboard

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
