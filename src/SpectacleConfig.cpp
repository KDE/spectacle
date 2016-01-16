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
