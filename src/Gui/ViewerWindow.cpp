/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ViewerWindow.h"

#include "Config.h"
#include "Gui/ExportMenu.h"
#include "InlineMessageModel.h"
#include "SpectacleCore.h"

#include <KUrlMimeData>
#include <Kirigami/Platform/Units>

#include <QApplication>
#include <QClipboard>
#include <QDrag>
#include <QFile>
#include <QMimeData>
#include <QShortcut>

using namespace Qt::StringLiterals;

ViewerWindow *ViewerWindow::s_viewerWindowInstance = nullptr;

ViewerWindow::ViewerWindow(Mode mode, QQmlEngine *engine, QWindow *parent)
    : SpectacleWindow(engine, parent)
    , m_mode(mode)
{
    s_viewerWindowInstance = this;
    s_isAnnotating = false;

    // Set up shortcuts. We won't need to access these again and the memory will be managed by Qt.
    new QShortcut(QKeySequence::Save, this, this, &ViewerWindow::save);
    new QShortcut(QKeySequence::SaveAs, this, this, &ViewerWindow::saveAs);
    new QShortcut(QKeySequence::Copy, this, this, &ViewerWindow::copyImage);
    new QShortcut(QKeySequence::Print, this, this, &ViewerWindow::showPrintDialog);

    m_context->setContextObject(this); // Must be before QML is initialized

    connect(ExportMenu::instance(), &ExportMenu::imageShared, this, [this](int error, const QString &message) {
        if (error == 1 || status() != QQuickView::Ready) {
            // error == 1 means the user cancelled the sharing
            return;
        }

        if (error) {
            auto text = i18nc("@info, %1 is the error message", "There was a problem sharing the image: %1", message);
            InlineMessageModel::instance()->push(InlineMessageModel::Error, text);
        } else {
            auto text = i18nc("@info", "The shared image link (<a href=\"%1\">%1</a>) has been copied to the clipboard.", message);
            InlineMessageModel::instance()->push(InlineMessageModel::Shared, text);
            if (!message.isEmpty()) {
                QApplication::clipboard()->setText(message);
            }
        }
    });

    // set up QML
    setResizeMode(QQuickView::SizeRootObjectToView);
    setMode(mode); // sets source and other stuff based on mode.
    m_oldWindowStates = windowStates();
}

ViewerWindow::~ViewerWindow()
{
    InlineMessageModel::instance()->clear();
    if (s_viewerWindowInstance == this) {
        s_viewerWindowInstance = nullptr;
    }
}

ViewerWindow::UniquePointer ViewerWindow::makeUnique(Mode mode, QQmlEngine *engine, QWindow *parent)
{
    return UniquePointer(new ViewerWindow(mode, engine, parent), [](ViewerWindow *window){
        s_viewerWindowInstance = nullptr;
        deleter(window);
    });
}

ViewerWindow *ViewerWindow::instance()
{
    return s_viewerWindowInstance;
}

void ViewerWindow::setMode(ViewerWindow::Mode mode)
{
    if (mode == Dialog) {
        setBackgroundColorRole(QPalette::Window);
        QVariantMap initialProperties = {
            // Set the parent in initialProperties to avoid having
            // the parent and window be null in Component.onCompleted
            {u"parent"_s, QVariant::fromValue(contentItem())}
        };
        setSource(QUrl("%1/Gui/DialogPage.qml"_L1.arg(SPECTACLE_QML_PATH)), initialProperties);
        auto rootItem = rootObject();
        if (!rootItem) {
            return;
        }
        const QSize implicitSize = {qRound(rootItem->implicitWidth()),
                                    qRound(rootItem->implicitHeight())};
        setMinimumSize(implicitSize);
        setMaximumSize(implicitSize);
        connect(rootItem, &QQuickItem::implicitWidthChanged, this, [this](){
            int implicitWidth = qRound(rootObject()->implicitWidth());
            setMinimumWidth(implicitWidth);
            setMaximumWidth(implicitWidth);
        });
        connect(rootItem, &QQuickItem::implicitHeightChanged, this, [this](){
            int implicitHeight = qRound(rootObject()->implicitHeight());
            setMinimumHeight(implicitHeight);
            setMaximumHeight(implicitHeight);
        });
    } else if (mode == Viewer) {
        setBackgroundColorRole(QPalette::Base);
        QVariantMap initialProperties = {
            // Set the parent in initialProperties to avoid having
            // the parent and window be null in Component.onCompleted
            {u"parent"_s, QVariant::fromValue(contentItem())}
        };
        setSource(QUrl("%1/Gui/ViewerPage.qml"_L1.arg(SPECTACLE_QML_PATH)), initialProperties);
        auto rootItem = rootObject();
        if (!rootItem) {
            return;
        }
        updateMinimumSize();
        connect(rootItem, SIGNAL(minimumWidthChanged()), this, SLOT(updateMinimumSize()));
        connect(rootItem, SIGNAL(minimumHeightChanged()), this, SLOT(updateMinimumSize()));
    }
}

void ViewerWindow::updateColor()
{
    setColor(qGuiApp->palette().color(m_backgroundColorRole));
}

void ViewerWindow::setBackgroundColorRole(QPalette::ColorRole role)
{
    m_backgroundColorRole = role;
    updateColor();
}

void ViewerWindow::updateMinimumSize()
{
    if (auto rootItem = rootObject()) {
        const QSize size = {qRound(rootItem->property("minimumWidth").toReal()),
                            qRound(rootItem->property("minimumHeight").toReal())};
        // Resizing should be automatic, but sometimes a manual resizing is needed
        // to fix window sizing/graphical glitches after a minimum size change.
        if (size.width() > width()) {
            setWidth(size.width());
        }
        if (size.height() > height()) {
            setHeight(size.height());
        }
        setMinimumSize(size);
    }
}

void ViewerWindow::startDrag()
{
    if (m_mode == Dialog) {
        return;
    }

    SpectacleCore::instance()->syncExportImage();
    auto exportManager = ExportManager::instance();
    const auto &image = exportManager->image();

    const QUrl tempFile = SpectacleCore::instance()->videoMode() ? SpectacleCore::instance()->currentVideo() : exportManager->tempSave();
    if (!tempFile.isValid()) {
        return;
    }

    auto mimeData = new QMimeData;
    mimeData->setUrls(QList<QUrl>{tempFile});
    // "application/x-kde-suggestedfilename" is handled by KIO/PasteJob.
    // It is only used when QMimeData::formats() is empty or when the user is
    // prompted to set a filename for the content after drag & drop or paste.
    // When QMimeData::formats() is empty, a dialog for picking the data format
    // is supposed to appear.
    // It's likely that users will never see the data format dialog with Spectacle.
    mimeData->setData(u"application/x-kde-suggestedfilename"_s, QFile::encodeName(tempFile.fileName()));
    KUrlMimeData::exportUrlsToPortal(mimeData);

    auto dragHandler = new QDrag(this);
    dragHandler->setMimeData(mimeData);

    if (SpectacleCore::instance()->videoMode()) {
        auto units = engine()->singletonInstance<Kirigami::Platform::Units *>("org.kde.kirigami.platform", "Units");
        auto iconSize = units->iconSizes()->large();
        dragHandler->setPixmap(QIcon::fromTheme(u"video-x-generic"_s).pixmap(iconSize, iconSize));
    } else {
        QSize size = image.size();
        QPixmap pixmap = QPixmap::fromImage(image);
        // TODO: use the composed pixmap with annotations instead
        if (size.width() > 256 || size.height() > 256) {
            dragHandler->setPixmap(pixmap.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            dragHandler->setPixmap(pixmap);
        }
    }
    dragHandler->exec(Qt::CopyAction);
}

bool ViewerWindow::event(QEvent *event)
{
    if (event->type() == QEvent::ApplicationPaletteChange) {
        updateColor();
    }
    return SpectacleWindow::event(event);
}

void ViewerWindow::resizeEvent(QResizeEvent *event)
{
    SpectacleWindow::resizeEvent(event);
    if (auto rootItem = rootObject()) {
        // sometimes rootObject size doesn't keep up with the window size
        rootItem->setSize(this->size());
    }
}

#include "moc_ViewerWindow.cpp"
