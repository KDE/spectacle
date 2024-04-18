/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "CaptureWindow.h"

#include "Config.h"
#include "SpectacleCore.h"
#include "Gui/SelectionEditor.h"

#include <QScreen>

using namespace Qt::StringLiterals;

QList<CaptureWindow *> CaptureWindow::s_captureWindowInstances = {};

CaptureWindow::CaptureWindow(Mode mode, QScreen *screen, QQmlEngine *engine, QWindow *parent)
    : SpectacleWindow(engine, parent)
    , m_screenToFollow(screen)
{
    s_captureWindowInstances.append(this);
    s_isAnnotating = true;

    m_context->setContextObject(this); // Must be before QML is initialized

    setFlags({
        Qt::Window, // the default window flag
        Qt::FramelessWindowHint,
        Qt::NoDropShadowWindowHint,
        Qt::MaximizeUsingFullscreenGeometryHint // also use the areas where system UIs are
    });

    setWindowStates(Qt::WindowFullScreen);

    this->setColor(Qt::transparent);

    // follow a screen
    connect(screen, &QScreen::geometryChanged, this, &CaptureWindow::syncGeometryWithScreen);
    connect(screen, &QScreen::physicalDotsPerInchChanged,
            this, &CaptureWindow::syncGeometryWithScreen);
    syncGeometryWithScreen();
    Q_EMIT screenToFollowChanged();

    // sync visibility
    connect(this, &QWindow::visibilityChanged, this, [this](QWindow::Visibility visibility){
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
    return UniquePointer(new CaptureWindow(mode, screen, engine, parent), [](CaptureWindow *window){
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

void CaptureWindow::setMode(CaptureWindow::Mode mode)
{
    if (mode == Image) {
        syncGeometryWithScreen();
        QVariantMap initialProperties = {
            // Set the parent in initialProperties to avoid having
            // the parent and window be null in Component.onCompleted
            {u"parent"_s, QVariant::fromValue(contentItem())}
        };
        setSource(QUrl("%1/Gui/ImageCaptureOverlay.qml"_L1.arg(SPECTACLE_QML_PATH)),
                  initialProperties);
    } else if (mode == Video) {
        syncGeometryWithScreen();
        QVariantMap initialProperties = {
            // Set the parent in initialProperties to avoid having
            // the parent and window be null in Component.onCompleted
            {u"parent"_s, QVariant::fromValue(contentItem())}
        };
        setSource(QUrl("%1/Gui/VideoCaptureOverlay.qml"_L1.arg(SPECTACLE_QML_PATH)),
                  initialProperties);
    }
}

bool CaptureWindow::accept()
{
    return SelectionEditor::instance()->acceptSelection();
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

#include "moc_CaptureWindow.cpp"
