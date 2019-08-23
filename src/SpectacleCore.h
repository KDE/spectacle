/* This file is part of Spectacle, the KDE screenshot utility
 * Copyright (C) 2019 Boudhayan Gupta <bgupta@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QObject>

#include "ExportManager.h"
#include "Gui/KSMainWindow.h"
#include "QuickEditor/QuickEditor.h"
#include "Platforms/PlatformLoader.h"

#include <memory>

namespace KWayland {
namespace Client {
class PlasmaShell;
}
}

using MainWindowPtr = std::unique_ptr<KSMainWindow>;
using EditorPtr     = std::unique_ptr<QuickEditor>;

class SpectacleCore: public QObject
{
    Q_OBJECT

    public:

    enum class StartMode {
        Gui        = 0,
        DBus       = 1,
        Background = 2
    };

    explicit SpectacleCore(StartMode theStartMode,
                           Spectacle::CaptureMode theCaptureMode,
                           QString &theSaveFileName,
                           qint64 theDelayMsec,
                           bool theNotifyOnGrab,
                           bool theCopyToClipboard,
                           QObject *parent = nullptr);
    virtual ~SpectacleCore() = default;

    QString filename() const;
    void setFilename(const QString &filename);

    Q_SIGNALS:

    void errorMessage(const QString &errString);
    void allDone();
    void filenameChanged(const QString &filename);
    void grabFailed();

    public Q_SLOTS:

    void takeNewScreenshot(Spectacle::CaptureMode theCaptureMode, int theTimeout, bool theIncludePointer, bool theIncludeDecorations);
    void showErrorMessage(const QString &theErrString);
    void screenshotUpdated(const QPixmap &thePixmap);
    void screenshotFailed();
    void dbusStartAgent();
    void doStartDragAndDrop();
    void doNotify(const QUrl &theSavedAt);
    void doCopyPath(const QUrl &savedAt);

    private:


    void initGui(bool theIncludePointer, bool theIncludeDecorations);
    Platform::GrabMode toPlatformGrabMode(Spectacle::CaptureMode theCaptureMode);
    void setUpShortcuts();

    StartMode     mStartMode;
    bool          mNotify;
    QString       mFileNameString;
    QUrl          mFileNameUrl;
    PlatformPtr   mPlatform;
    MainWindowPtr mMainWindow;
    EditorPtr     mQuickEditor;
    bool          mIsGuiInited;
    bool          mCopyToClipboard;
    KWayland::Client::PlasmaShell *mWaylandPlasmashell;
};
