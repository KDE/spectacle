/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 * SPDX-FileCopyrightText: 2020 Ahmad Samir <a.samirh78@gmail.com>
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SpectacleWindow.h"

#include "ExportManager.h"
#include "SpectacleCore.h"
#include "Geometry.h"
#include "Gui/ExportMenu.h"
#include "Gui/HelpMenu.h"
#include "Gui/OptionsMenu.h"
#include "Gui/WidgetWindowUtils.h"
#include "spectacle_gui_debug.h"

#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenFileManagerWindowJob>
#include <KWindowSystem>

#include <QApplication>
#include <QColorDialog>
#include <QFontDialog>
#include <QtQml>
#include <utility>

using namespace Qt::StringLiterals;
using G = Geometry;

QList<SpectacleWindow *> SpectacleWindow::s_spectacleWindowInstances = {};
bool SpectacleWindow::s_synchronizingVisibility = false;
bool SpectacleWindow::s_synchronizingTitle = false;
SpectacleWindow::TitlePreset SpectacleWindow::s_lastTitlePreset = Default;
QString SpectacleWindow::s_previousTitle = QGuiApplication::applicationDisplayName();
bool SpectacleWindow::s_synchronizingAnnotating = false;
bool SpectacleWindow::s_isAnnotating = false;

SpectacleWindow::SpectacleWindow(QQmlEngine *engine, QWindow *parent)
    : QQuickView(engine, parent)
    , m_context(new QQmlContext(engine->rootContext(), this))
{
    s_spectacleWindowInstances.append(this);

    connect(engine, &QQmlEngine::quit, QCoreApplication::instance(), &QCoreApplication::quit, Qt::QueuedConnection);
    connect(this, &QQuickView::statusChanged, this, [](QQuickView::Status status){
        if (status == QQuickView::Error) {
            QCoreApplication::quit();
        }
    });
    connect(this, &SpectacleWindow::xChanged, this, &SpectacleWindow::logicalXChanged);
    connect(this, &SpectacleWindow::yChanged, this, &SpectacleWindow::logicalYChanged);

    setTextRenderType(QQuickWindow::NativeTextRendering);

    // set up QML
    setResizeMode(QQuickView::SizeRootObjectToView);
    m_context->setContextProperty(u"contextWindow"_s, this);
}

SpectacleWindow::~SpectacleWindow()
{
    s_spectacleWindowInstances.removeOne(this);
}

qreal SpectacleWindow::logicalX() const
{
    return G::mapFromPlatformValue(x(), devicePixelRatio());
}

qreal SpectacleWindow::logicalY() const
{
    return G::mapFromPlatformValue(y(), devicePixelRatio());
}

bool SpectacleWindow::isAnnotating() const
{
    return s_isAnnotating;
}

void SpectacleWindow::setAnnotating(bool annotating)
{
    if (s_synchronizingAnnotating || s_isAnnotating == annotating) {
        return;
    }
    s_synchronizingAnnotating = true;
    s_isAnnotating = annotating;
    for (auto window : std::as_const(s_spectacleWindowInstances)) {
        Q_EMIT window->annotatingChanged();
    }
    s_synchronizingAnnotating = false;
}

void SpectacleWindow::unminimize()
{
    setVisible(true);
    setWindowStates(windowStates().setFlag(Qt::WindowMinimized, false));
}

QList<SpectacleWindow *> SpectacleWindow::instances()
{
    return s_spectacleWindowInstances;
}

void SpectacleWindow::setVisibilityForAll(QWindow::Visibility visibility)
{
    if (s_synchronizingVisibility || s_spectacleWindowInstances.isEmpty()) {
        return;
    }
    s_synchronizingVisibility = true;
    for (auto window : std::as_const(s_spectacleWindowInstances)) {
        window->setVisibility(visibility);
    }
    s_synchronizingVisibility = false;
}

void SpectacleWindow::setTitleForAll(TitlePreset preset, const QString &fileName)
{
    if (s_synchronizingTitle || s_spectacleWindowInstances.isEmpty()) {
        return;
    }
    s_synchronizingTitle = true;

    QString newTitle = titlePresetString(preset, fileName);

    if (!newTitle.isEmpty()) {
        if (s_lastTitlePreset != TitlePreset::Timer) {
            s_previousTitle = s_spectacleWindowInstances.constFirst()->title();
        }
        s_lastTitlePreset = preset;

        for (auto window : std::as_const(s_spectacleWindowInstances)) {
            window->setTitle(newTitle);
        }
    }

    s_synchronizingTitle = false;
}

void SpectacleWindow::closeAll()
{
    // counting down should prevent invalid memory access
    for (int i = s_spectacleWindowInstances.count() - 1; i >= 0; --i) {
        s_spectacleWindowInstances[i]->close();
    }
}

qreal SpectacleWindow::dprRound(qreal value) const
{
    return G::dprRound(value, devicePixelRatio());
}

qreal SpectacleWindow::dprCeil(qreal value) const
{
    return G::dprCeil(value, devicePixelRatio());
}

qreal SpectacleWindow::dprFloor(qreal value) const
{
    return G::dprFloor(value, devicePixelRatio());
}

QString SpectacleWindow::baseFileName(const QUrl &url) const
{
    return url.fileName();
}

QString SpectacleWindow::titlePresetString(TitlePreset preset, const QString &fileName)
{
    if (preset == TitlePreset::Timer) {
        return i18ncp("@title:window", "%1 second", "%1 seconds",
                      qCeil(SpectacleCore::instance()->captureTimeRemaining() / 1000.0));
    } else if (preset == TitlePreset::Unsaved) {
        return i18nc("@title:window Unsaved Screenshot", "Unsaved") + u"*"_s;
    } else if (preset == TitlePreset::Saved && !fileName.isEmpty()) {
        return fileName;
    } else if (preset == TitlePreset::Modified && !fileName.isEmpty()) {
        return fileName + u"*"_s;
    } else if (preset == TitlePreset::Previous && !s_previousTitle.isEmpty()) {
        return s_previousTitle;
    }
    return QGuiApplication::applicationDisplayName();
}

void SpectacleWindow::deleter(SpectacleWindow *window)
{
    s_spectacleWindowInstances.removeOne(window);
    window->deleteLater();
}

void SpectacleWindow::setSource(const QUrl &source, const QVariantMap &initialProperties)
{
    if (source.isEmpty()) {
        m_component.reset(nullptr);
        QQuickView::setSource(source);
        return;
    }

    m_component.reset(new QQmlComponent(engine(), source, this));
    auto *component = m_component.get();
    QObject *object = nullptr;

    if (component->isLoading()) {
        connect(component, &QQmlComponent::statusChanged,
                this, [this, component, &source, &initialProperties]() {
            disconnect(component, &QQmlComponent::statusChanged, this, nullptr);
            QObject *object = nullptr;
            if (component->isReady()) {
                if (!initialProperties.isEmpty()) {
                    object = component->createWithInitialProperties(initialProperties,
                                                                    m_context.get());
                } else {
                    object = component->create(m_context.get());
                }
            }
            setContent(source, component, object);
        });
    } else if (component->isReady()) {
        if (!initialProperties.isEmpty()) {
            object = component->createWithInitialProperties(initialProperties, m_context.get());
        } else {
            object = component->create(m_context.get());
        }
    }

    setContent(source, component, object);
}

void SpectacleWindow::save()
{
    SpectacleCore::instance()->syncExportImage();
    ExportManager::instance()->exportImage(ExportManager::Save | ExportManager::UserAction,
                                           SpectacleCore::instance()->outputUrl());
}

void SpectacleWindow::saveAs()
{
    if (SpectacleCore::instance()->videoMode()) {
        ExportManager::instance()->exportVideo(ExportManager::SaveAs | ExportManager::UserAction,
                                               SpectacleCore::instance()->currentVideo());
        return;
    }
    SpectacleCore::instance()->syncExportImage();
    ExportManager::instance()->exportImage(ExportManager::SaveAs | ExportManager::UserAction);
}

void SpectacleWindow::copyImage()
{
    SpectacleCore::instance()->syncExportImage();
    ExportManager::instance()->exportImage(ExportManager::CopyImage | ExportManager::UserAction);
}

void SpectacleWindow::copyLocation()
{
    if (SpectacleCore::instance()->videoMode()) {
        ExportManager::instance()->exportVideo(ExportManager::CopyPath | ExportManager::UserAction,
                                               SpectacleCore::instance()->currentVideo());
        return;
    }
    SpectacleCore::instance()->syncExportImage();
    ExportManager::instance()->exportImage(ExportManager::CopyPath | ExportManager::UserAction);
}

void SpectacleWindow::showPrintDialog()
{
    SpectacleCore::instance()->syncExportImage();
    ExportMenu::instance()->openPrintDialog();
}

void SpectacleWindow::showPreferencesDialog()
{
    OptionsMenu::instance()->showPreferencesDialog();
}

void SpectacleWindow::showFontDialog()
{
    auto tool = SpectacleCore::instance()->annotationDocument()->tool();
    auto saWrapper = SpectacleCore::instance()->annotationDocument()->selectedActionWrapper();
    QFont font;
    if (tool->type() == AnnotationDocument::ChangeAction
        || (tool->type() == AnnotationDocument::Text
            && saWrapper->type() == AnnotationDocument::Text)
    ) {
        font = saWrapper->font();
    } else {
        font = tool->font();
    }
    QFontDialog *dialog = new QFontDialog(font);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    setWidgetTransientParent(dialog, this);

    if (flags().testFlag(Qt::WindowStaysOnTopHint)) {
        dialog->setWindowFlag(Qt::WindowStaysOnTopHint);
    }

    connect(dialog, &QFontDialog::fontSelected, this, [](const QFont &font) {
        QFont newFont = font;
        // Copied from stripRegularStyleName() in KFontChooserDialog.
        // For more details see:
        // https://bugreports.qt.io/browse/QTBUG-63792
        // https://bugs.kde.org/show_bug.cgi?id=378523
        if (newFont.weight() == QFont::Normal
            && (newFont.styleName() == "Regular"_L1
                || newFont.styleName() == "Normal"_L1
                || newFont.styleName() == "Book"_L1
                || newFont.styleName() == "Roman"_L1)) {
            newFont.setStyleName(QString());
        }
        auto tool = SpectacleCore::instance()->annotationDocument()->tool();
        auto saWrapper = SpectacleCore::instance()->annotationDocument()->selectedActionWrapper();
        if (tool->type() == AnnotationDocument::ChangeAction) {
            saWrapper->setFont(newFont);
            saWrapper->commitChanges();
        } else if (tool->type() == AnnotationDocument::Text
            && saWrapper->type() == AnnotationDocument::Text
        ) {
            tool->setFont(newFont);
            saWrapper->setFont(newFont);
            saWrapper->commitChanges();
        } else {
            tool->setFont(newFont);
        }
    });

    // BUG https://bugs.kde.org/show_bug.cgi?id=478155:
    // Workaround modal font dialog being unusable.
    // This should probably be fixed in the plasma-integration.
    dialog->setModal(false);
    dialog->show();
}

void SpectacleWindow::showColorDialog(int option)
{
    QColorDialog *dialog = nullptr;
    auto tool = SpectacleCore::instance()->annotationDocument()->tool();
    auto saWrapper = SpectacleCore::instance()->annotationDocument()->selectedActionWrapper();

    std::function<QColor()> toolGetter;
    std::function<QColor()> sawGetter;
    std::function<void(const QColor &)> toolSetter;
    std::function<void(const QColor &)> sawSetter;
    using namespace std::placeholders; // for std::placeholders::_1
    if (option == AnnotationTool::Stroke) {
        toolGetter = std::bind(&AnnotationTool::strokeColor, tool);
        sawGetter = std::bind(&SelectedActionWrapper::strokeColor, saWrapper);
        toolSetter = std::bind(&AnnotationTool::setStrokeColor, tool, _1);
        sawSetter = std::bind(&SelectedActionWrapper::setStrokeColor, saWrapper, _1);
    } else if (option == AnnotationTool::Fill) {
        toolGetter = std::bind(&AnnotationTool::fillColor, tool);
        sawGetter = std::bind(&SelectedActionWrapper::fillColor, saWrapper);
        toolSetter = std::bind(&AnnotationTool::setFillColor, tool, _1);
        sawSetter = std::bind(&SelectedActionWrapper::setFillColor, saWrapper, _1);
    } else if (option == AnnotationTool::Font) {
        toolGetter = std::bind(&AnnotationTool::fontColor, tool);
        sawGetter = std::bind(&SelectedActionWrapper::fontColor, saWrapper);
        toolSetter = std::bind(&AnnotationTool::setFontColor, tool, _1);
        sawSetter = std::bind(&SelectedActionWrapper::setFontColor, saWrapper, _1);
    } else {
        qmlWarning(this) << "invalid option argument";
        return;
    }

    QColor color;
    if (tool->type() == AnnotationDocument::ChangeAction
        || (tool->type() == AnnotationDocument::Text
            && saWrapper->type() == AnnotationDocument::Text)
    ) {
        color = sawGetter();
    } else {
        color = toolGetter();
    }

    dialog = new QColorDialog(color);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setOption(QColorDialog::ShowAlphaChannel);

    setWidgetTransientParent(dialog, this);

    if (flags().testFlag(Qt::WindowStaysOnTopHint)) {
        dialog->setWindowFlag(Qt::WindowStaysOnTopHint);
    }

    connect(dialog, &QColorDialog::colorSelected, this, [toolSetter, sawSetter](const QColor &color){
        auto tool = SpectacleCore::instance()->annotationDocument()->tool();
        auto saw = SpectacleCore::instance()->annotationDocument()->selectedActionWrapper();
        if (tool->type() == AnnotationDocument::ChangeAction) {
            sawSetter(color);
            saw->commitChanges();
        } else if (tool->type() == AnnotationDocument::Text
            && saw->type() == AnnotationDocument::Text
        ) {
            toolSetter(color);
            sawSetter(color);
            saw->commitChanges();
        } else {
            toolSetter(color);
        }
    });

    dialog->open();
}

void SpectacleWindow::openContainingFolder(const QUrl &url)
{
    KIO::highlightInFileManager({url});
}

void SpectacleWindow::mousePressEvent(QMouseEvent *event)
{
    // QMenus need to be closed by hand when used from QML, see plasma-workspace/shellcorona.cpp
    if (auto popup = QApplication::activePopupWidget()) {
        popup->close();
        event->accept();
    } else {
        QQuickView::mousePressEvent(event);
    }
}

void SpectacleWindow::keyPressEvent(QKeyEvent *event)
{
    // Events need to be processed normally first for events to reach items
    QQuickView::keyPressEvent(event);
    if (event->isAccepted()) {
        return;
    }
    m_pressedKeys = event->key() | event->modifiers();
}

void SpectacleWindow::keyReleaseEvent(QKeyEvent *event)
{
    // Events need to be processed normally first for events to reach items
    QQuickView::keyReleaseEvent(event);
    if (event->isAccepted()) {
        return;
    }
    // Cancel defaults to Escape in QPlatformTheme.
    // Handling this here fixes https://bugs.kde.org/show_bug.cgi?id=428478
    if ((event->matches(QKeySequence::Quit)
        || event->matches(QKeySequence::Close)
        || event->matches(QKeySequence::Cancel))
        // We need to check if these were pressed previously or else pressing escape
        // in a dialog will quit spectacle when you release the escape key.
        && m_pressedKeys == event->key() | event->modifiers()
    ) {
        event->accept();
        auto spectacleCore = SpectacleCore::instance();
        spectacleCore->cancelScreenshot();
    } else if (event->matches(QKeySequence::Preferences)) {
        event->accept();
        showPreferencesDialog();
    } else if (event->matches(QKeySequence::New)) {
        event->accept();
        SpectacleCore::instance()->takeNewScreenshot();
    } else if (event->matches(QKeySequence::HelpContents)) {
        event->accept();
        HelpMenu::instance()->showAppHelp();
    }
    m_pressedKeys = {};
}

#include "moc_SpectacleWindow.cpp"
