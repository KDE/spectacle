/*
    SPDX-FileCopyrightText: 2021 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "ImagePlatform.h"

#include <QDBusUnixFileDescriptor>
#include <QFuture>
#include <QImage>
class QScreen;

#include <memory>

class ScreenShotSource2;
class ScreenShotSourceMeta2;
class ScreenShotSourceWorkspace2;

// Needed for ResultVariant to compile with Qt meta object stuff.
inline bool operator<(const QImage &lhs, const QImage &rhs)
{
    return lhs.cacheKey() < rhs.cacheKey();
}

struct ResultVariant : public std::variant<std::monostate, QImage, QString> {
    enum Type : std::size_t {
        CanceledState,
        Image,
        ErrorString,
        NPos = std::variant_npos,
    };
    static ResultVariant canceled()
    {
        return {std::monostate{}};
    };
};

/**
 * The PlatformKWin class uses the org.kde.KWin.ScreenShot2 dbus interface
 * for taking screenshots of screens and windows.
 */
class ImagePlatformKWin final : public ImagePlatform
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.spectacle.ImagePlatform" FILE "metadata.json")
    Q_INTERFACES(ImagePlatform)

public:
    explicit ImagePlatformKWin(QObject *parent = nullptr);
    ~ImagePlatformKWin() override = default;

    enum class ScreenShotFlag : uint {
        IncludeCursor = 0x1,
        IncludeDecoration = 0x2,
        NativeSize = 0x4,
        IncludeShadow = 0x8,
    };
    Q_DECLARE_FLAGS(ScreenShotFlags, ScreenShotFlag)

    enum class InteractiveKind : uint {
        Window = 0,
        Screen = 1,
    };

    GrabModes supportedGrabModes() const override;
    ShutterModes supportedShutterModes() const override;

public Q_SLOTS:
    void
    doGrab(ImagePlatform::ShutterMode shutterMode, ImagePlatform::GrabMode grabMode, bool includePointer, bool includeDecorations, bool includeShadow) override;

private Q_SLOTS:
    void updateSupportedGrabModes();

private:
    void takeScreenShotInteractive(InteractiveKind kind, ScreenShotFlags flags);
    void takeScreenShotArea(const QRect &area, ScreenShotFlags flags);
    void takeScreenShotActiveWindow(ScreenShotFlags flags);
    void takeScreenShotActiveScreen(ScreenShotFlags flags);
    void takeScreenShotScreens(const QList<QScreen *> &screens, ScreenShotFlags flags);
    void takeScreenShotWorkspace(ScreenShotFlags flags);
    void takeScreenShotCroppable(ScreenShotFlags flags);

    void trackSource(ScreenShotSource2 *source);
    template<typename OutputSignal>
    void trackSource(ScreenShotSourceMeta2 *source, OutputSignal outputSignal);

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

Q_SIGNALS:
    void finished(const ResultVariant &result);

private:
    QDBusUnixFileDescriptor m_pipeFileDescriptor;
};

/**
 * The ScreenShotSourceArea2 class provides a convenient way to take a screenshot of the
 * specified area using the org.kde.KWin.ScreenShot2 dbus interface.
 */
class ScreenShotSourceArea2 final : public ScreenShotSource2
{
    Q_OBJECT

public:
    ScreenShotSourceArea2(const QRect &area, ImagePlatformKWin::ScreenShotFlags flags);
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
    ScreenShotSourceInteractive2(ImagePlatformKWin::InteractiveKind kind, ImagePlatformKWin::ScreenShotFlags flags);
};

/**
 * The ScreenShotSourceScreen2 class provides a convenient way to take a screenshot of
 * the specified screen using the org.kde.KWin.ScreenShot2 dbus interface.
 */
class ScreenShotSourceScreen2 final : public ScreenShotSource2
{
    Q_OBJECT

public:
    ScreenShotSourceScreen2(const QScreen *screen, ImagePlatformKWin::ScreenShotFlags flags);
};

/**
 * The ScreenShotSourceActiveWindow2 class provides a convenient way to take a screenshot
 * of the active window. This uses the org.kde.KWin.ScreenShot2 dbus interface.
 */
class ScreenShotSourceActiveWindow2 final : public ScreenShotSource2
{
    Q_OBJECT

public:
    ScreenShotSourceActiveWindow2(ImagePlatformKWin::ScreenShotFlags flags);
};

/**
 * The ScreenShotSourceActiveScreen2 class provides a convenient way to take a screenshot
 * of the active screen. This uses the org.kde.KWin.ScreenShot2 dbus interface.
 */
class ScreenShotSourceActiveScreen2 final : public ScreenShotSource2
{
    Q_OBJECT

public:
    ScreenShotSourceActiveScreen2(ImagePlatformKWin::ScreenShotFlags flags);
};

/**
 * The ScreenShotSourceMeta2 class represents a screenshot source that is made of several
 * other sources.
 */
class ScreenShotSourceMeta2 final : public QObject
{
    Q_OBJECT

public:
    explicit ScreenShotSourceMeta2(const QList<ScreenShotSource2 *> &sources);

Q_SIGNALS:
    void finished(const QList<ResultVariant> &results);

private:
    QList<ResultVariant> m_results;
};

/**
 * The ScreenShotSourceWorkspace2 class provides a convenient way to take a screenshot
 * of the whole workspace. This uses the org.kde.KWin.ScreenShot2 dbus interface.
 */
class ScreenShotSourceWorkspace2 final : public ScreenShotSource2
{
    Q_OBJECT

public:
    ScreenShotSourceWorkspace2(ImagePlatformKWin::ScreenShotFlags flags);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ImagePlatformKWin::ScreenShotFlags)
