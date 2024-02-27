/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

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
    Q_PROPERTY(bool annotating READ isAnnotating WRITE setAnnotating NOTIFY annotatingChanged FINAL)
    Q_PROPERTY(qreal logicalX READ logicalX NOTIFY logicalXChanged)
    Q_PROPERTY(qreal logicalY READ logicalY NOTIFY logicalYChanged)

public:
    enum TitlePreset {
        Default,
        Timer,
        Unsaved,
        Saved,
        Modified,
        Previous,
    };

    qreal logicalX() const;
    qreal logicalY() const;

    bool isAnnotating() const;
    void setAnnotating(bool annotating);

    /**
     * Makes the window visible and removes the WindowMinimized flag from the WindowStates flags.
     */
    void unminimize();

    static QList<SpectacleWindow *> instances();

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
     * Close all SpectacleWindows.
     */
    static void closeAll();

    /**
     * Round value to be physically pixel perfect, based on the device pixel ratio.
     * Meant to be used with coordinates, line widths and shape sizes.
     * This is meant to be used in QML.
     */
    Q_INVOKABLE qreal dprRound(qreal value) const;
    Q_INVOKABLE qreal dprCeil(qreal value) const;
    Q_INVOKABLE qreal dprFloor(qreal value) const;

    /**
     * Get the basename for a file URL.
     * This is meant to be used in QML.
     */
    Q_INVOKABLE QString baseFileName(const QUrl &url) const;

public Q_SLOTS:
    virtual void save();
    virtual void saveAs();
    virtual void copyImage();
    virtual void copyLocation();
    virtual void copyToClipboard(const QVariant &content);
    void showPrintDialog();
    void showPreferencesDialog();
    void showFontDialog();
    void showColorDialog(int option);
    void openContainingFolder(const QUrl &url);

Q_SIGNALS:
    void annotatingChanged();
    void logicalXChanged();
    void logicalYChanged();

protected:
    explicit SpectacleWindow(QQmlEngine *engine, QWindow *parent = nullptr);
    ~SpectacleWindow();

    using QQuickView::setTitle;

    static QString titlePresetString(TitlePreset preset, const QString &fileName = {});
    static void deleter(SpectacleWindow *window);

    // set source, but with a window specific QQmlContext and initial properties
    void setSource(const QUrl &source, const QVariantMap &initialProperties);

    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    static QList<SpectacleWindow *> s_spectacleWindowInstances;
    static bool s_synchronizingVisibility;
    static bool s_synchronizingTitle;
    static TitlePreset s_lastTitlePreset;
    static QString s_previousTitle;
    static bool s_synchronizingAnnotating;
    static bool s_isAnnotating;

    const std::unique_ptr<QQmlContext> m_context;
    std::unique_ptr<QQmlComponent> m_component;

    QKeySequence m_pressedKeys;
};
