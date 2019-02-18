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

#ifndef SPECTACLECONFIG_H
#define SPECTACLECONFIG_H

#include <QObject>
#include <QUrl>
#include <QRect>

#include <KSharedConfig>
#include <KConfigGroup>

enum class SaveMode {
    SaveAs,
    Save
};

class SpectacleConfig : public QObject
{
    Q_OBJECT

    // singleton-ize the class

    public:

    static SpectacleConfig* instance();

    QString defaultFilename() const;
    QString defaultTimestampTemplate() const;
    
    QUrl lastSaveAsLocation() const;
    QUrl lastSaveLocation() const;
    
    private:

    explicit SpectacleConfig(QObject *parent = nullptr);
    virtual ~SpectacleConfig();

    SpectacleConfig(SpectacleConfig const&) = delete;
    void operator= (SpectacleConfig const&) = delete;

    // everything else

    public Q_SLOTS:

    QUrl lastSaveAsFile() const;
    void setLastSaveAsFile(const QUrl &location);

    QUrl lastSaveFile() const;
    void setLastSaveFile(const QUrl &location);

    QRect cropRegion() const;
    void setCropRegion(const QRect &region);

    bool onClickChecked() const;
    void setOnClickChecked(bool enabled);

    bool includePointerChecked() const;
    void setIncludePointerChecked(bool enabled);

    bool includeDecorationsChecked() const;
    void setIncludeDecorationsChecked(bool enabled);

    bool captureTransientWindowOnlyChecked() const;
    void setCaptureTransientWindowOnlyChecked(bool enabled);

    bool quitAfterSaveOrCopyChecked() const;
    void setQuitAfterSaveOrCopyChecked(bool enabled);

    bool showMagnifierChecked() const;
    void setShowMagnifierChecked(bool enabled);

    qreal captureDelay() const;
    void setCaptureDelay(qreal delay);

    int captureMode() const;
    void setCaptureMode(int index);

    bool rememberLastRectangularRegion() const;
    void setRememberLastRectangularRegion(bool enabled);

    bool alwaysRememberRegion() const;
    void setAlwaysRememberRegion(bool enabled);

    bool useLightRegionMaskColour() const;
    void setUseLightRegionMaskColour(bool enabled);

    SaveMode lastUsedSaveMode() const;
    void setLastUsedSaveMode(SaveMode mode);

    QString autoSaveFilenameFormat() const;
    void setAutoSaveFilenameFormat(const QString &format);

    QString defaultSaveLocation() const;
    void setDefaultSaveLocation(const QString &location);

    bool copySaveLocationToClipboard() const;
    void setCopySaveLocationToClipboard(bool enabled);

    QString saveImageFormat() const;
    void setSaveImageFormat(const QString &saveFmt);

    private:

    KSharedConfigPtr mConfig;
    KConfigGroup     mGeneralConfig;
    KConfigGroup     mGuiConfig;
};

#endif // SPECTACLECONFIG_H
