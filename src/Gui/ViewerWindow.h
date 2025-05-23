/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "Gui/SpectacleWindow.h"
#include <QPalette>

class ViewerWindowPrivate;

/**
 * The window used for viewing media after it has been accepted or finished recording.
 * This has to be a separate window from the selection/capture window because reusing
 * the same window and changing the flags doesn't work nicely on Wayland. For example,
 * the window uses default window decorations instead of normal decorations.
 */
class ViewerWindow : public SpectacleWindow
{
    Q_OBJECT
public:
    enum Mode {
        Dialog,
        Viewer,
    };

    using UniquePointer = std::unique_ptr<ViewerWindow, void (*)(ViewerWindow *)>;

    static UniquePointer makeUnique(Mode mode, QQmlEngine *engine, QWindow *parent = nullptr);

    static ViewerWindow *instance();

    Q_INVOKABLE void startDrag();

protected:
    bool event(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    explicit ViewerWindow(Mode mode, QQmlEngine *engine, QWindow *parent = nullptr);
    ~ViewerWindow();

    void setMode(ViewerWindow::Mode mode);
    Q_SLOT void updateColor();
    Q_SLOT void updateMinimumSize();

    void setBackgroundColorRole(QPalette::ColorRole role);

    bool m_pixmapExists = false;
    QPalette::ColorRole m_backgroundColorRole;
    Qt::WindowStates m_oldWindowStates;
    const Mode m_mode;
    static ViewerWindow *s_viewerWindowInstance;
};
