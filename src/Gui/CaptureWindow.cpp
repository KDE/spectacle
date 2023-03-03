/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "CaptureWindow.h"

#include "Config.h"
#include "Gui/SpectacleWindow.h"
#include "SpectacleCore.h"
#include "Gui/SelectionEditor.h"
#include "spectacle_gui_debug.h"

CaptureWindow::CaptureWindow(Mode mode, QScreen *screen, QQmlEngine *engine, QWindow *parent)
    : SpectacleWindow(engine, parent)
    , m_screenToFollow(screen)
{
    s_isAnnotating = true;

    setFlags({
        Qt::Window, // the default window flag
        Qt::FramelessWindowHint,
        Qt::NoDropShadowWindowHint,
        Qt::MaximizeUsingFullscreenGeometryHint // also use the areas where system UIs are
    });

    setWindowStates(Qt::WindowFullScreen);

    this->setColor(Qt::transparent);

    // setup selectionEditor
    auto selectionEditor = SelectionEditor::instance();
    connect(selectionEditor, &SelectionEditor::screensRectChanged, this, [this]() {
        syncGeometryWithScreen();
    });
    connect(selectionEditor, &SelectionEditor::screenImagesChanged, this, &CaptureWindow::screenCaptureUrlChanged);
    connect(this, &CaptureWindow::devicePixelRatioChanged,
            selectionEditor, &SelectionEditor::devicePixelRatioChanged);

    // set up QML
    setMode(mode); // sets source and other stuff based on mode.
    if (auto rootItem = rootObject()) {
        rootItem->installEventFilter(selectionEditor);
    }

    // follow a screen
    connect(screen, &QScreen::geometryChanged, this, &CaptureWindow::syncGeometryWithScreen);
    connect(screen, &QScreen::physicalDotsPerInchChanged, this, [this]() {
        Q_EMIT devicePixelRatioChanged(m_screenToFollow->devicePixelRatio());
        syncGeometryWithScreen();
    });
    syncGeometryWithScreen();
    Q_EMIT screenToFollowChanged();
    Q_EMIT devicePixelRatioChanged(m_screenToFollow->devicePixelRatio());
    Q_EMIT screenCaptureUrlChanged();

    // sync visibility
    connect(this, &QWindow::visibilityChanged, this, [this](QWindow::Visibility visibility){
        if (s_synchronizingVisibility || s_instances.size() <= 1) {
            return;
        }
        s_synchronizingVisibility = true;
        for (auto window : std::as_const(s_instances)) {
            if (window == this) {
                continue;
            }
            window->setVisibility(visibility);
        }
        s_synchronizingVisibility = false;
    });
}

CaptureWindow::~CaptureWindow()
{
    if (auto rootItem = rootObject()) {
        rootItem->removeEventFilter(SelectionEditor::instance());
    }
}

CaptureWindow::UniquePointer CaptureWindow::makeUnique(Mode mode, QScreen *screen, QQmlEngine *engine, QWindow *parent)
{
    return UniquePointer(new CaptureWindow(mode, screen, engine, parent),
                         &SpectacleWindow::deleter);
}

QScreen *CaptureWindow::screenToFollow() const
{
    return m_screenToFollow;
}

QString CaptureWindow::screenCaptureUrl() const
{
    if (!m_screenToFollow) {
        return QString();
    }
    return QStringLiteral("image://spectacle/screen/") + m_screenToFollow->name() + QLatin1Char('/')
        + QString::number(SelectionEditor::instance()->imageForScreen(m_screenToFollow).cacheKey());
}

void CaptureWindow::setMode(CaptureWindow::Mode mode)
{
    if (mode == Image) {
        syncGeometryWithScreen();
        QVariantMap initialProperties = {
            // Set the parent in initialProperties to avoid having
            // the parent and window be null in Component.onCompleted
            {QStringLiteral("parent"), QVariant::fromValue(contentItem())}
        };
        setSource(QUrl(QStringLiteral("qrc:/src/Gui/ImageCaptureOverlay.qml")), initialProperties);
    } else if (mode == Video) {
        
    }
}

bool CaptureWindow::accept()
{
    return SelectionEditor::instance()->acceptSelection();
}

void CaptureWindow::save()
{
    accept();
    SpectacleWindow::save();
}

void CaptureWindow::saveAs()
{
    accept();
    SpectacleWindow::saveAs();
}

void CaptureWindow::copyImage()
{
    accept();
    SpectacleWindow::copyImage();
}

void CaptureWindow::copyLocation()
{
    accept();
    SpectacleWindow::copyLocation();
}

void CaptureWindow::mousePressEvent(QMouseEvent *event)
{
    requestActivate();
    SpectacleWindow::mousePressEvent(event);
}

void CaptureWindow::keyReleaseEvent(QKeyEvent *event)
{
    SpectacleWindow::keyReleaseEvent(event);
    if (event->isAccepted()) {
        return;
    }
    if (event->matches(QKeySequence::Save)) {
        event->accept();
        save();
    } else if (event->matches(QKeySequence::SaveAs)) {
        event->accept();
        saveAs();
    } else if (event->matches(QKeySequence::Copy)) {
        event->accept();
        copyImage();
    } else if (event->matches(QKeySequence::Print)) {
        event->accept();
        showPrintDialog();
    }
    auto document = SpectacleCore::instance()->annotationDocument();
    if (!event->isAccepted() && document) {
        if (document->undoStackDepth() > 0 && event->matches(QKeySequence::Undo)) {
            event->accept();
            document->undo();
        } else if (document->redoStackDepth() > 0 && event->matches(QKeySequence::Redo)) {
            event->accept();
            document->redo();
        }
    }
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
