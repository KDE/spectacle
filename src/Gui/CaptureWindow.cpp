/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "CaptureWindow.h"

#include "Config.h"
#include "Gui/SelectionEditor.h"
#include "SpectacleCore.h"

#include <KWaylandExtras>

#include <QApplication>
#include <QClipboard>
#include <QScreen>
#include <QShortcut>

using namespace Qt::StringLiterals;

QList<CaptureWindow *> CaptureWindow::s_captureWindowInstances = {};
qreal CaptureWindow::s_maxDevicePixelRatio = 1;

CaptureWindow::CaptureWindow(Mode mode, QScreen *screen, QQmlEngine *engine, QWindow *parent)
    : SpectacleWindow(engine, parent)
    , m_screenToFollow(screen)
{
    s_captureWindowInstances.append(this);
    s_isAnnotating = true;

    // Set up shortcuts. We won't need to access these again and the memory will be managed by Qt.
    new QShortcut(QKeySequence::Save, this, this, &CaptureWindow::save);
    new QShortcut(QKeySequence::SaveAs, this, this, &CaptureWindow::saveAs);
    new QShortcut(QKeySequence::Copy, this, this, &CaptureWindow::copyImage);
    new QShortcut(QKeySequence::Print, this, this, &CaptureWindow::showPrintDialog);

    m_context->setContextObject(this); // Must be before QML is initialized

    setFlags({
        Qt::Window, // the default window flag
        Qt::FramelessWindowHint,
        Qt::NoDropShadowWindowHint,
        Qt::MaximizeUsingFullscreenGeometryHint // also use the areas where system UIs are
    });

    setWindowStates(Qt::WindowFullScreen);

    KWaylandExtras::setXdgToplevelTag(this, QStringLiteral("region-editor"));

    this->setColor(Qt::transparent);

    // follow a screen
    connect(screen, &QScreen::geometryChanged, this, &CaptureWindow::syncGeometryWithScreen);
    connect(screen, &QScreen::physicalDotsPerInchChanged, this, &CaptureWindow::syncGeometryWithScreen);
    syncGeometryWithScreen();
    Q_EMIT screenToFollowChanged();
    // BUG: https://bugs.kde.org/show_bug.cgi?id=502047
    // Workaround window choosing wrong screen with some fractional DPR screen layout combinations.
    connect(this, &QWindow::screenChanged, this, [this] {
        syncGeometryWithScreen();
    });

    // sync visibility
    connect(this, &QWindow::visibilityChanged, this, [this](QWindow::Visibility visibility) {
        if (s_synchronizingVisibility || s_captureWindowInstances.size() <= 1) {
            return;
        }
        s_synchronizingVisibility = true;
        for (auto window : std::as_const(s_captureWindowInstances)) {
            if (window == this) {
                continue;
            }
            window->setVisibility(visibility);
        }
        s_synchronizingVisibility = false;
    });

    // setup selectionEditor
    auto selectionEditor = SelectionEditor::instance();
    connect(selectionEditor, &SelectionEditor::screensRectChanged, this, [this]() {
        syncGeometryWithScreen();
    });

    // set up QML
    setMode(mode); // sets source and other stuff based on mode.
    if (auto rootItem = rootObject()) {
        rootItem->installEventFilter(selectionEditor);
    }
}

CaptureWindow::~CaptureWindow()
{
    s_captureWindowInstances.removeOne(this);
    if (auto rootItem = rootObject()) {
        rootItem->removeEventFilter(SelectionEditor::instance());
    }
}

CaptureWindow::UniquePointer CaptureWindow::makeUnique(Mode mode, QScreen *screen, QQmlEngine *engine, QWindow *parent)
{
    return UniquePointer(new CaptureWindow(mode, screen, engine, parent), [](CaptureWindow *window) {
        s_captureWindowInstances.removeOne(window);
        deleter(window);
    });
}

QList<CaptureWindow *> CaptureWindow::instances()
{
    return s_captureWindowInstances;
}

QScreen *CaptureWindow::screenToFollow() const
{
    return m_screenToFollow;
}

qreal CaptureWindow::maxDevicePixelRatio()
{
    return s_maxDevicePixelRatio;
}

void CaptureWindow::setMode(CaptureWindow::Mode mode)
{
    syncGeometryWithScreen();
    QVariantMap initialProperties = {// Set the parent in initialProperties to avoid having
                                     // the parent and window be null in Component.onCompleted
                                     {u"parent"_s, QVariant::fromValue(contentItem())}};
    setSource(QUrl("%1/Gui/CaptureOverlay.qml"_L1.arg(SPECTACLE_QML_PATH)), initialProperties);
}

bool CaptureWindow::accept()
{
    return SelectionEditor::instance()->acceptSelection();
}

void CaptureWindow::cancel()
{
    SpectacleCore::instance()->cancelScreenshot();
}

void CaptureWindow::save()
{
    SelectionEditor::instance()->acceptSelection(ExportManager::Save | ExportManager::UserAction);
}

void CaptureWindow::saveAs()
{
    SelectionEditor::instance()->acceptSelection(ExportManager::SaveAs | ExportManager::UserAction);
}

void CaptureWindow::copyImage()
{
    SelectionEditor::instance()->acceptSelection(ExportManager::CopyImage | ExportManager::UserAction);
}

void CaptureWindow::copyLocation()
{
    SelectionEditor::instance()->acceptSelection(ExportManager::Save | ExportManager::CopyPath | ExportManager::UserAction);
}

void CaptureWindow::exposeEvent(QExposeEvent *event)
{
    SpectacleWindow::exposeEvent(event);
    if (!isExposed()) {
        return;
    }
    qreal maxDpr = 0;
    int windowsExposed = 0;
    const auto windows = CaptureWindow::instances();
    for (auto window : windows) {
        if (!window->isExposed()) {
            return;
        }
        ++windowsExposed;
        maxDpr = std::max(maxDpr, window->devicePixelRatio());
    }
    if (windowsExposed == windows.size()) {
        const bool maxDprChanged = s_maxDevicePixelRatio != maxDpr;
        s_maxDevicePixelRatio = maxDpr;
        for (auto window : windows) {
            if (maxDprChanged) {
                Q_EMIT window->maxDevicePixelRatioChanged();
            }
            Q_EMIT window->allExposed();
        }
    }
}

void CaptureWindow::mousePressEvent(QMouseEvent *event)
{
    requestActivate();
    SpectacleWindow::mousePressEvent(event);
}

void CaptureWindow::showEvent(QShowEvent *event)
{
    SpectacleWindow::showEvent(event);
    syncGeometryWithScreen();
    requestActivate();
}

void CaptureWindow::syncGeometryWithScreen()
{
    if (!m_screenToFollow) {
        return;
    }

    QRect screenRect = m_screenToFollow->geometry();

    // Set minimum size to ensure the window always covers the area it is meant to.
    setMinimumSize(screenRect.size());
    setMaximumSize(screenRect.size());
    setGeometry(screenRect);
    setScreen(m_screenToFollow);
}

#include "moc_CaptureWindow.cpp"
