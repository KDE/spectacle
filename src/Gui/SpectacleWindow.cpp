/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 * SPDX-FileCopyrightText: 2020 Ahmad Samir <a.samirh78@gmail.com>
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SpectacleWindow.h"

#include "SpectacleCore.h"
#include "spectacle_gui_debug.h"

#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenFileManagerWindowJob>
#include <KIO/OpenUrlJob>
#include <KUrlMimeData>
#include <KWayland/Client/surface.h>
#include <KWindowSystem>

#include <QApplication>
#include <QColorDialog>
#include <QDrag>
#include <QFontDialog>
#include <QtQml>
#include <utility>

#ifdef XCB_FOUND
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QX11Info>
#else
#include <private/qtx11extras_p.h>
#endif
#endif

QVector<SpectacleWindow *> SpectacleWindow::s_instances = {};
bool SpectacleWindow::s_synchronizingVisibility = false;
bool SpectacleWindow::s_synchronizingTitle = false;
SpectacleWindow::TitlePreset SpectacleWindow::s_lastTitlePreset = Default;
QString SpectacleWindow::s_previousTitle = QGuiApplication::applicationDisplayName();

SpectacleWindow::SpectacleWindow(QQmlEngine *engine, QWindow *parent)
    : QQuickView(engine, parent)
    , m_exportMenu(new ExportMenu)
    , m_optionsMenu(new OptionsMenu)
    , m_helpMenu(new HelpMenu)
    , m_context(new QQmlContext(engine->rootContext(), this))
{
    s_instances.append(this);

    if (m_exportMenu->winId()) {
        m_exportMenu->windowHandle()->setTransientParent(this);
    }
    if (m_optionsMenu->winId()) {
        m_optionsMenu->windowHandle()->setTransientParent(this);
    }
    if (m_helpMenu->winId()) {
        m_helpMenu->windowHandle()->setTransientParent(this);
    }
    connect(engine, &QQmlEngine::quit, QCoreApplication::instance(), &QCoreApplication::quit, Qt::QueuedConnection);
    connect(this, &QQuickView::statusChanged, this, [](QQuickView::Status status){
        if (status == QQuickView::Error) {
            QCoreApplication::quit();
        }
    });

    // before we do anything, we need to set a window property
    // that skips the close/hide window animation on kwin. this
    // fixes a ghost image of the spectacle window that appears
    // on subsequent screenshots taken with the take new screenshot
    // button
    //
    // credits for this goes to Thomas Lübking <thomas.luebking@gmail.com>

#ifdef XCB_FOUND
    if (KWindowSystem::isPlatformX11()) {
        // do the xcb shenanigans
        xcb_connection_t *xcbConn = QX11Info::connection();
        const QByteArray effectName = QByteArrayLiteral("_KDE_NET_WM_SKIP_CLOSE_ANIMATION");

        xcb_intern_atom_cookie_t atomCookie = xcb_intern_atom_unchecked(xcbConn, false, effectName.length(), effectName.constData());
        QScopedPointer<xcb_intern_atom_reply_t, QScopedPointerPodDeleter> atom(xcb_intern_atom_reply(xcbConn, atomCookie, nullptr));
        if (!atom.isNull()) {
            uint32_t value = 1;
            xcb_change_property(xcbConn, XCB_PROP_MODE_REPLACE, winId(), atom->atom, XCB_ATOM_CARDINAL, 32, 1, &value);
        }
    }
#endif

    setTextRenderType(QQuickWindow::NativeTextRendering);

    // set up QML
    setResizeMode(QQuickView::SizeRootObjectToView);
    m_context->setContextProperty(QStringLiteral("contextWindow"), this);
}

SpectacleWindow::~SpectacleWindow()
{
    s_instances.removeOne(this);
}

ExportMenu *SpectacleWindow::exportMenu() const
{
    return m_exportMenu.get();
}

OptionsMenu *SpectacleWindow::optionsMenu() const
{
    return m_optionsMenu.get();
}

HelpMenu *SpectacleWindow::helpMenu() const
{
    return m_helpMenu.get();
}

void SpectacleWindow::unminimize()
{
    setVisible(true);
    setWindowStates(windowStates().setFlag(Qt::WindowMinimized, false));
}

QVector<SpectacleWindow *> SpectacleWindow::instances()
{
    return s_instances;
}

void SpectacleWindow::setVisibilityForAll(QWindow::Visibility visibility)
{
    if (s_synchronizingVisibility || s_instances.isEmpty()) {
        return;
    }
    s_synchronizingVisibility = true;
    for (auto window : s_instances) {
        window->setVisibility(visibility);
    }
    s_synchronizingVisibility = false;
}

void SpectacleWindow::setTitleForAll(TitlePreset preset, const QString &fileName)
{
    if (s_synchronizingTitle || s_instances.isEmpty()) {
        return;
    }
    s_synchronizingTitle = true;

    QString newTitle = titlePresetString(preset, fileName);

    if (!newTitle.isEmpty()) {
        if (s_lastTitlePreset != TitlePreset::Timer) {
            s_previousTitle = s_instances.constFirst()->title();
        }
        s_lastTitlePreset = preset;

        for (auto window : s_instances) {
            window->setTitle(newTitle);
        }
    }

    s_synchronizingTitle = false;
}

qreal SpectacleWindow::dprRound(qreal value) const
{
    return std::round(value * devicePixelRatio()) / devicePixelRatio();
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
        return i18nc("@title:window Unsaved Screenshot", "Unsaved") + QStringLiteral("*");
    } else if (preset == TitlePreset::Saved && !fileName.isEmpty()) {
        return fileName;
    } else if (preset == TitlePreset::Modified && !fileName.isEmpty()) {
        return fileName + QStringLiteral("*");
    } else if (preset == TitlePreset::Previous && !s_previousTitle.isEmpty()) {
        return s_previousTitle;
    }
    return QGuiApplication::applicationDisplayName();
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

void SpectacleWindow::setPosition(const QPoint &p)
{
    using namespace KWayland::Client;
    // TODO This is a hack until a better interface is available.
    // Original context: https://phabricator.kde.org/D23466
    if (auto surface = plasmashellSurface()) {
        surface->setPosition(p);
    } else {
        QQuickView::setPosition(p);
    }
}

void SpectacleWindow::setGeometry(const QRect &r)
{
    QQuickView::setGeometry(r);
    using namespace KWayland::Client;
    // TODO This is a hack until a better interface is available.
    // Original context: https://phabricator.kde.org/D23466
    if (auto surface = plasmashellSurface()) {
        surface->setPosition(r.topLeft());
    }
}

void SpectacleWindow::save()
{
    const bool quitChecked = Settings::quitAfterSaveCopyExport();
    // emits ExportManager::forceNotify when quitChecked is true,
    // which is connected to SpectacleCore::doNotify,
    // which emits SpectacleCore::allDone,
    // which is connected to QCoreApplication::quit in Main.cpp
    ExportManager::instance()->doSave(QUrl(), /* notify */ quitChecked);
    if (quitChecked) {
        qApp->setQuitOnLastWindowClosed(false);
        SpectacleWindow::setVisibilityForAll(QWindow::Hidden);
    }
}

void SpectacleWindow::saveAs()
{
    const bool quitChecked = Settings::quitAfterSaveCopyExport();
    if (ExportManager::instance()->doSaveAs(/* notify */ quitChecked) && quitChecked) {
        qApp->setQuitOnLastWindowClosed(false);
        SpectacleWindow::setVisibilityForAll(QWindow::Hidden);
    }
}

void SpectacleWindow::copyImage()
{
    const bool quitChecked = Settings::quitAfterSaveCopyExport();
    SpectacleCore::instance()->syncExportPixmap();
    ExportManager::instance()->doCopyToClipboard(/* notify */ quitChecked);
    if (quitChecked) {
        qApp->setQuitOnLastWindowClosed(false);
        SpectacleWindow::setVisibilityForAll(QWindow::Hidden);
    }
}

void SpectacleWindow::copyLocation()
{
    const bool quitChecked = Settings::quitAfterSaveCopyExport();
    ExportManager::instance()->doCopyLocationToClipboard();
    if (quitChecked) {
        qApp->setQuitOnLastWindowClosed(false);
        SpectacleWindow::setVisibilityForAll(QWindow::Hidden);
    }
}

void SpectacleWindow::showPrintDialog()
{
    m_exportMenu->openPrintDialog();
}

void SpectacleWindow::showPreferencesDialog()
{
    m_optionsMenu->showPreferencesDialog();
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

    if (dialog->winId()) {
        dialog->windowHandle()->setTransientParent(this);
    }

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
            && (newFont.styleName() == QLatin1String("Regular")
                || newFont.styleName() == QLatin1String("Normal")
                || newFont.styleName() == QLatin1String("Book")
                || newFont.styleName() == QLatin1String("Roman"))) {
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

    dialog->open();
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

    if (dialog->winId()) {
        dialog->windowHandle()->setTransientParent(this);
    }

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

void SpectacleWindow::openUrlExternally(const QUrl &url)
{
    auto job = new KIO::OpenUrlJob(url);
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
    job->start();
}

void SpectacleWindow::openContainingFolder(const QUrl &url)
{
    KIO::highlightInFileManager({url});
}

void SpectacleWindow::startDrag()
{
    auto exportManager = ExportManager::instance();
    if (exportManager->pixmap().isNull()) {
        return;
    }

    QUrl tempFile = exportManager->tempSave();
    if (!tempFile.isValid()) {
        return;
    }

    auto mimeData = new QMimeData;
    mimeData->setUrls(QList<QUrl>{tempFile});
    mimeData->setData(QStringLiteral("application/x-kde-suggestedfilename"), QFile::encodeName(tempFile.fileName()));
    KUrlMimeData::exportUrlsToPortal(mimeData);

    auto dragHandler = new QDrag(this);
    dragHandler->setMimeData(mimeData);
    QSize size = exportManager->pixmap().size();

    // TODO: use the composed pixmap with annotations instead
    if (size.width() > 256 || size.height() > 256) {
        dragHandler->setPixmap(exportManager->pixmap().scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        dragHandler->setPixmap(exportManager->pixmap());
    }
    dragHandler->exec(Qt::CopyAction);
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
    // Cancel defaults to Escape in QPlatformTheme
    if (event->matches(QKeySequence::Quit)
        || event->matches(QKeySequence::Close)
        || event->matches(QKeySequence::Cancel)) {
        // we must do the shortcut here to prevent closing dialogs from closing Spectacle
        auto spectacleCore = SpectacleCore::instance();
        if (spectacleCore->captureTimeRemaining() > 0) {
            spectacleCore->cancelScreenshot();
        } else {
            engine()->quit();
        }
    }
}

void SpectacleWindow::keyReleaseEvent(QKeyEvent *event)
{
    // Events need to be processed normally first for events to reach items
    QQuickView::keyReleaseEvent(event);
    if (event->isAccepted()) {
        return;
    }
    if (event->matches(QKeySequence::Quit)
        || event->matches(QKeySequence::Close)
        || event->matches(QKeySequence::Cancel)) {
        event->accept();
    } else if (event->matches(QKeySequence::Preferences)) {
        event->accept();
        showPreferencesDialog();
    } else if (event->matches(QKeySequence::New)) {
        event->accept();
        SpectacleCore::instance()->takeNewScreenshot();
    } else if (event->matches(QKeySequence::HelpContents)) {
        event->accept();
        m_helpMenu->showAppHelp();
    }
}

KWayland::Client::PlasmaShellSurface *SpectacleWindow::plasmashellSurface()
{
    // TODO This is a hack until a better interface is available.
    // Original context: https://phabricator.kde.org/D23466
    if (auto plasmashell = SpectacleCore::instance()->plasmaShellInterfaceWrapper()) {
        using namespace KWayland::Client;
        auto surface = Surface::fromWindow(this);
        if (surface) {
            return plasmashell->createSurface(surface, this);
        }
    }
    return nullptr;
}
