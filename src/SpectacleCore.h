/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2019 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <QCommandLineParser>

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

    explicit SpectacleCore(QObject *parent = nullptr);
    virtual ~SpectacleCore() = default;
    void init();

    QString filename() const;
    void setFilename(const QString &filename);

    void populateCommandLineParser(QCommandLineParser *lCmdLineParser);

    Q_SIGNALS:

    void errorMessage(const QString &errString);
    void allDone();
    void grabFailed();

    public Q_SLOTS:

    void takeNewScreenshot(Spectacle::CaptureMode theCaptureMode, int theTimeout, bool theIncludePointer, bool theIncludeDecorations);
    void showErrorMessage(const QString &theErrString);
    void screenshotUpdated(const QPixmap &thePixmap);
    void screenshotsUpdated(const QVector<QImage> &imgs);
    void screenshotCanceled();
    void screenshotFailed();
    void doStartDragAndDrop();
    void doNotify(const QUrl &theSavedAt);
    void doCopyPath(const QUrl &savedAt);

    void onActivateRequested(QStringList arguments, const QString& /*workingDirectory */);

    private:

    void initGui(int theDelay, bool theIncludePointer, bool theIncludeDecorations);
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
    bool          mCopyImageToClipboard;
    bool          mCopyLocationToClipboard;
    bool          mSaveToOutput;
    KWayland::Client::PlasmaShell *mWaylandPlasmashell;
};
