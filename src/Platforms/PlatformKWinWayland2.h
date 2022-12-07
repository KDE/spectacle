/*
    SPDX-FileCopyrightText: 2021 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "Platform.h"

#include <QImage>
class QScreen;

#include <memory>

class ScreenShotSource2;
class ScreenShotSourceMeta2;

/**
 * The PlatformKWinWayland2 class uses the org.kde.KWin.ScreenShot2 dbus interface
 * for taking screenshots of screens and windows.
 */
class PlatformKWinWayland2 final : public Platform
{
    Q_OBJECT

public:
    static std::unique_ptr<PlatformKWinWayland2> create();

    enum class ScreenShotFlag : uint {
        IncludeCursor = 0x1,
        IncludeDecoration = 0x2,
        NativeSize = 0x4,
    };
    Q_DECLARE_FLAGS(ScreenShotFlags, ScreenShotFlag)

    enum class InteractiveKind : uint {
        Window = 0,
        Screen = 1,
    };

    QString platformName() const override;
    GrabModes supportedGrabModes() const override;
    ShutterModes supportedShutterModes() const override;

public Q_SLOTS:
    void doGrab(Platform::ShutterMode theShutterMode, Platform::GrabMode theGrabMode, bool theIncludePointer, bool theIncludeDecorations) override;

private Q_SLOTS:
    void updateSupportedGrabModes();

private:
    explicit PlatformKWinWayland2(QObject *parent = nullptr);

    void takeScreenShotInteractive(InteractiveKind kind, ScreenShotFlags flags);
    void takeScreenShotArea(const QRect &area, ScreenShotFlags flags);
    void takeScreenShotScreens(const QList<QScreen *> &screens, ScreenShotFlags flags);
    void takeScreenShotActiveWindow(ScreenShotFlags flags);
    void takeScreenShotActiveScreen(ScreenShotFlags flags);

    void trackSource(ScreenShotSource2 *source);
    void trackSource(ScreenShotSourceMeta2 *source);

    int m_apiVersion = 1;
    GrabModes m_grabModes;
};

/**
 * The ScreenShotSource2 class is the base class for screenshot sources that use the
 * org.kde.KWin.ScreenShot2 dbus interface.
 */
class ScreenShotSource2 : public QObject
{
    Q_OBJECT

public:
    template<typename... ArgType>
    explicit ScreenShotSource2(const QString &methodName, ArgType... arguments);
    ~ScreenShotSource2() override;

    QImage result() const;

Q_SIGNALS:
    void finished(const QImage &image);
    void errorOccurred();

private Q_SLOTS:
    void handleMetaDataReceived(const QVariantMap &metadata);

private:
    QImage m_result;
    int m_pipeFileDescriptor = -1;
};

/**
 * The ScreenShotSourceArea2 class provides a convenient way to take a screenshot of the
 * specified area using the org.kde.KWin.ScreenShot2 dbus interface.
 */
class ScreenShotSourceArea2 final : public ScreenShotSource2
{
    Q_OBJECT

public:
    ScreenShotSourceArea2(const QRect &area, PlatformKWinWayland2::ScreenShotFlags flags);
};

/**
 * The ScreenShotSourceInteractive2 class provides a convenient way to take a screenshot
 * of a screen or a window as selected by the user. This uses the org.kde.KWin.ScreenShot2
 * dbus interface.
 */
class ScreenShotSourceInteractive2 final : public ScreenShotSource2
{
    Q_OBJECT

public:
    ScreenShotSourceInteractive2(PlatformKWinWayland2::InteractiveKind kind, PlatformKWinWayland2::ScreenShotFlags flags);
};

/**
 * The ScreenShotSourceScreen2 class provides a convenient way to take a screenshot of
 * the specified screen using the org.kde.KWin.ScreenShot2 dbus interface.
 */
class ScreenShotSourceScreen2 final : public ScreenShotSource2
{
    Q_OBJECT

public:
    ScreenShotSourceScreen2(const QScreen *screen, PlatformKWinWayland2::ScreenShotFlags flags);
};

/**
 * The ScreenShotSourceActiveWindow2 class provides a convenient way to take a screenshot
 * of the active window. This uses the org.kde.KWin.ScreenShot2 dbus interface.
 */
class ScreenShotSourceActiveWindow2 final : public ScreenShotSource2
{
    Q_OBJECT

public:
    ScreenShotSourceActiveWindow2(PlatformKWinWayland2::ScreenShotFlags flags);
};

/**
 * The ScreenShotSourceActiveScreen2 class provides a convenient way to take a screenshot
 * of the active screen. This uses the org.kde.KWin.ScreenShot2 dbus interface.
 */
class ScreenShotSourceActiveScreen2 final : public ScreenShotSource2
{
    Q_OBJECT

public:
    ScreenShotSourceActiveScreen2(PlatformKWinWayland2::ScreenShotFlags flags);
};

/**
 * The ScreenShotSourceMeta2 class represents a screenshot source that is made of several
 * other sources.
 */
class ScreenShotSourceMeta2 final : public QObject
{
    Q_OBJECT

public:
    explicit ScreenShotSourceMeta2(const QVector<ScreenShotSource2 *> &sources);

Q_SIGNALS:
    void finished(const QVector<QImage> &images);
    void errorOccurred();

private Q_SLOTS:
    void handleSourceFinished();
    void handleSourceErrorOccurred();

private:
    QVector<ScreenShotSource2 *> m_sources;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(PlatformKWinWayland2::ScreenShotFlags)
