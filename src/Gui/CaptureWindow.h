/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "Gui/SpectacleWindow.h"

class CaptureWindowPrivate;

/**
 * The window class used for fullscreen capture UIs.
 */
class CaptureWindow : public SpectacleWindow
{
    Q_OBJECT
    Q_PROPERTY(QScreen *screenToFollow READ screenToFollow NOTIFY screenToFollowChanged FINAL)
    Q_PROPERTY(QString screenCaptureUrl READ screenCaptureUrl NOTIFY screenCaptureUrlChanged FINAL)

public:
    enum Mode {
        Image,
        Video,
    };

    using UniquePointer = std::unique_ptr<CaptureWindow, decltype(&SpectacleWindow::deleter)>;

    static UniquePointer makeUnique(Mode mode, QScreen *screen, QQmlEngine *engine, QWindow *parent = nullptr);

    QScreen *screenToFollow() const;

    QString screenCaptureUrl() const;

public Q_SLOTS:
    bool accept();
    void save() override;
    void saveAs() override;
    void copyImage() override;
    void copyLocation() override;

Q_SIGNALS:
    void screenToFollowChanged();
    void devicePixelRatioChanged(qreal devicePixelRatio);
    void screenCaptureUrlChanged();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    explicit CaptureWindow(Mode mode, QScreen *screen, QQmlEngine *engine, QWindow *parent = nullptr);
    ~CaptureWindow();

    void setMode(CaptureWindow::Mode mode);
    void syncGeometryWithScreen();

    QPointer<QScreen> m_screenToFollow;
};
