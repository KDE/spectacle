/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "SpectacleCore.h"
#include <QDBusAbstractAdaptor>

class SpectacleDBusAdapter : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Spectacle")
public:
    SpectacleDBusAdapter(SpectacleCore *parent);
    ~SpectacleDBusAdapter() override = default;

    inline SpectacleCore *parent() const;

public Q_SLOTS:

    Q_NOREPLY void FullScreen(int includeMousePointer);
    Q_NOREPLY void CurrentScreen(int includeMousePointer);
    Q_NOREPLY void ActiveWindow(int includeWindowDecorations, int includeMousePointer);
    Q_NOREPLY void WindowUnderCursor(int includeWindowDecorations, int includeMousePointer);
    Q_NOREPLY void RectangularRegion(int includeMousePointer);
    Q_NOREPLY void OpenWithoutScreenshot();

Q_SIGNALS:

    void ScreenshotTaken(const QString &fileName);
    void ScreenshotFailed();
};
