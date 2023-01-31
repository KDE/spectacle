/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "Gui/ExportMenu.h"
#include "Gui/HelpMenu.h"
#include "Gui/OptionsMenu.h"

#include <KWayland/Client/plasmashell.h>

#include <QQuickView>
#include <QQmlContext>

class SpectacleWindowPrivate;

/**
 * The base window class for Spectacle's Qt Quick UIs.
 * Adapted from KSMainWindow, a QDialog subclass from the old Qt Widgets UI.
 */
class SpectacleWindow : public QQuickView
{
    Q_OBJECT
    Q_PROPERTY(ExportMenu *exportMenu READ exportMenu CONSTANT FINAL)
    Q_PROPERTY(OptionsMenu *optionsMenu READ optionsMenu CONSTANT FINAL)
    Q_PROPERTY(HelpMenu *helpMenu READ helpMenu CONSTANT FINAL)
    Q_PROPERTY(bool annotating READ isAnnotating WRITE setAnnotating NOTIFY annotatingChanged FINAL)

public:
    explicit SpectacleWindow(QQmlEngine *engine, QWindow *parent = nullptr);
    ~SpectacleWindow();

    enum TitlePreset {
        Default,
        Timer,
        Unsaved,
        Saved,
        Modified,
        Previous,
    };

    ExportMenu *exportMenu() const;
    OptionsMenu *optionsMenu() const;
    HelpMenu *helpMenu() const;

    bool isAnnotating() const;
    void setAnnotating(bool annotating);

    /**
     * Makes the window visible and removes the WindowMinimized flag from the WindowStates flags.
     */
    void unminimize();

    static QVector<SpectacleWindow *> instances();

    /**
     * Set the visibility of all SpectacleWindows created in SpectacleCore.
     * This will not work until the windows are fully initialized in SpectacleCore.
     */
    static void setVisibilityForAll(QWindow::Visibility visibility);

    /**
     * For all SpectacleWindows created in SpectacleCore, set the title based on the chosen preset.
     * The `fileName` parameter is used for the Saved and Modified presets.
     * This will not work until the windows are fully initialized in SpectacleCore.
     */
    static void setTitleForAll(TitlePreset preset, const QString &fileName = {});

    /**
     * Round value to be physically pixel perfect, based on the device pixel ratio.
     * Meant to be used with coordinates, line widths and shape sizes.
     * This is meant to be used in QML.
     */
    Q_INVOKABLE qreal dprRound(qreal value) const;

    /**
     * Get the basename for a file URL.
     * This is meant to be used in QML.
     */
    Q_INVOKABLE QString baseFileName(const QUrl &url) const;

public Q_SLOTS:
    // QWindow::setPosition has no effect on wayland, so here's one that works
    virtual void setPosition(const QPoint &p);
    virtual void setGeometry(const QRect &r);
    virtual void save();
    virtual void saveAs();
    virtual void copyImage();
    virtual void copyLocation();
    void showPrintDialog();
    void showPreferencesDialog();
    void showFontDialog();
    void showColorDialog(int option);
    // TODO: Remove in Qt6. Qt.openUrlExternally() doesn't activate the window on wayland.
    void openUrlExternally(const QUrl &url);
    void openContainingFolder(const QUrl &url);
    void startDrag();

Q_SIGNALS:
    void annotatingChanged();

protected:
    using QQuickView::setTitle;

    static QString titlePresetString(TitlePreset preset, const QString &fileName = {});

    // set source, but with a window specific QQmlContext and initial properties
    void setSource(const QUrl &source, const QVariantMap &initialProperties);

    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    KWayland::Client::PlasmaShellSurface *plasmashellSurface();

    KWayland::Client::PlasmaShellSurface *m_plasmaShellSurface = nullptr;

    static QVector<SpectacleWindow *> s_instances;
    static bool s_synchronizingVisibility;
    static bool s_synchronizingTitle;
    static TitlePreset s_lastTitlePreset;
    static QString s_previousTitle;
    static bool s_synchronizingAnnotating;
    static bool s_isAnnotating;

    const std::unique_ptr<ExportMenu> m_exportMenu;
    const std::unique_ptr<OptionsMenu> m_optionsMenu;
    const std::unique_ptr<HelpMenu> m_helpMenu;
    const std::unique_ptr<QQmlContext> m_context;
    std::unique_ptr<QQmlComponent> m_component;
};
