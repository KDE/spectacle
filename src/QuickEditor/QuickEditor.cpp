/*
 *  SPDX-FileCopyrightText: 2018 Ambareesh "Amby" Balaji <ambareeshbalaji@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "QuickEditor.h"
#include "AreaSelector.h"
#include "settings.h"

#include <KLocalizedString>
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>
#include <KWindowSystem>

#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QGridLayout>
#include <QGuiApplication>
#include <QOpenGLWidget>
#include <QPainter>
#include <QScreen>

QuickEditorView::QuickEditorView(QGraphicsScene *scene, KWayland::Client::PlasmaShell *plasmashell, QScreen *screen)
    : QGraphicsView(scene)
    , mDesiredScreen(screen)
{
    setViewport(new QOpenGLWidget());
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::BypassWindowManagerHint);
    setRenderHint(QPainter::Antialiasing);

    setFrameStyle(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // TODO This is a hack until a better interface is available
    if (plasmashell) {
        using namespace KWayland::Client;
        winId();
        auto surface = Surface::fromWindow(windowHandle());
        if (surface) {
            mPlasmaShellSurface.reset(plasmashell->createSurface(surface));
            mPlasmaShellSurface->setRole(PlasmaShellSurface::Role::Panel);
            mPlasmaShellSurface->setPanelTakesFocus(true);
        }
    }

    relayout();
    connect(screen, &QScreen::geometryChanged, this, &QuickEditorView::relayout);
}

QuickEditorView::~QuickEditorView()
{
}

void QuickEditorView::relayout()
{
    setSceneRect(mDesiredScreen->geometry());
    setGeometry(mDesiredScreen->geometry());
    if (mPlasmaShellSurface) {
        mPlasmaShellSurface->setPosition(mDesiredScreen->geometry().topLeft());
    }
}

QuickEditor::QuickEditor(const QMap<const QScreen *, QImage> &images, KWayland::Client::PlasmaShell *plasmashell, QObject *parent)
    : QObject(parent)
    , mImages(images)
    , mScene(new QGraphicsScene(this))
{
    const QList<QScreen *> screens = QGuiApplication::screens();

    // Put a monitor screenshot on each available screen. Should we monitor screen changes?
    for (QScreen *screen : screens) {
        QPixmap source = QPixmap::fromImage(images[screen]);
        source.setDevicePixelRatio(screen->devicePixelRatio());
        QGraphicsPixmapItem *pixmapItem = mScene->addPixmap(source);
        pixmapItem->setPos(screen->geometry().topLeft());

        QuickEditorView *view = new QuickEditorView(mScene, plasmashell, screen);
        view->show();
        mViews.append(view);
    }

    // An overlay item that provides a way to select an area on the screen.
    mSelectorItem = new AreaSelectorItem();
    mSelectorItem->setSelectionColor(QGuiApplication::palette().highlight().color());
    mSelectorItem->setBrush(Settings::useLightMaskColour() ? QColor(255, 255, 255, 100) : QColor(0, 0, 0, 38));
    mSelectorItem->setRect(mScene->sceneRect());

    if (!(Settings::rememberLastRectangularRegion() == Settings::EnumRememberLastRectangularRegion::Never)) {
        auto savedRect = Settings::cropRegion();
        const QRectF cropRegion(savedRect[0], savedRect[1], savedRect[2], savedRect[3]);
        if (!cropRegion.isEmpty()) {
            mSelectorItem->setSelection(cropRegion.intersected(mSelectorItem->rect()));
        }
    }
    mScene->addItem(mSelectorItem);
    connect(mSelectorItem, &AreaSelectorItem::selectionAccepted, this, &QuickEditor::captureSelection);
    connect(mSelectorItem, &AreaSelectorItem::selectionCanceled, this, &QuickEditor::grabCancelled);

    // A label showing the current size of the selection.
    QLabel *selectionSizeLabel = new QLabel();
    selectionSizeLabel->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum));
    selectionSizeLabel->setTextFormat(Qt::PlainText);
    selectionSizeLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    mScene->addWidget(selectionSizeLabel);
    auto updateSelectionSizeLabel = [this, selectionSizeLabel]() {
        const QRect selection = mSelectorItem->selection().toRect();
        selectionSizeLabel->setVisible(!selection.isEmpty());
        if (selectionSizeLabel->isVisible()) {
            selectionSizeLabel->setText(QStringLiteral("%1×%2").arg(selection.width()).arg(selection.height()));
            selectionSizeLabel->resize(selectionSizeLabel->sizeHint());
            selectionSizeLabel->move(selection.x() + (selection.width() - selectionSizeLabel->width()) / 2,
                                     selection.y() + (selection.height() - selectionSizeLabel->height()) / 2);
        }
    };
    updateSelectionSizeLabel();
    connect(mSelectorItem, &AreaSelectorItem::selectionChanged, selectionSizeLabel, updateSelectionSizeLabel);

    // An optional label explaining how to start selection.
    if (mSelectorItem->selection().isEmpty()) {
        QLabel *helpLabel = new QLabel();
        helpLabel->setText(i18n("Click and drag to draw a selection rectangle,\nor press Esc to quit"));
        helpLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

        QFont font = helpLabel->font();
        font.setPointSize(12);
        helpLabel->setFont(font);

        const QScreen *primaryScreen = QGuiApplication::primaryScreen();
        helpLabel->resize(helpLabel->sizeHint());
        helpLabel->move(primaryScreen->geometry().x() + (primaryScreen->geometry().width() - helpLabel->width()) / 2,
                        primaryScreen->geometry().y() + (primaryScreen->geometry().height() - helpLabel->height()) / 2);

        mScene->addWidget(helpLabel);
        connect(mSelectorItem, &AreaSelectorItem::selectionChanged, helpLabel, &QObject::deleteLater);
    }

    // A bubble with helpful information.
    QGridLayout *helpBoxGrid = new QGridLayout();
    QFrame *helpBox = new QFrame();
    helpBox->setLayout(helpBoxGrid);

    if (Settings::useReleaseToCapture() && mSelectorItem->selection().size().isEmpty()) {
        // Release to capture enabled and NO saved region available
        helpBoxGrid->addWidget(new QLabel(i18n("Take Screenshot:")), 0, 0, Qt::AlignRight);
        helpBoxGrid->addWidget(new QLabel(i18nc("Mouse action", "Release left-click")), 0, 1);
        helpBoxGrid->addWidget(new QLabel(i18nc("Keyboard action", "Enter")), 1, 1);

        helpBoxGrid->addWidget(new QLabel(i18n("Create new selection rectangle:")), 2, 0, Qt::AlignRight);
        helpBoxGrid->addWidget(new QLabel(i18nc("Mouse action", "Drag outside selection rectangle")), 2, 1);

        helpBoxGrid->addWidget(new QLabel(i18n("Cancel:")), 3, 0, Qt::AlignRight);
        helpBoxGrid->addWidget(new QLabel(i18nc("Keyboard action", "Escape")), 3, 1);
    } else {
        // Default text, Release to capture option disabled
        helpBoxGrid->addWidget(new QLabel(i18n("Take Screenshot:")), 0, 0, Qt::AlignRight);
        helpBoxGrid->addWidget(new QLabel(i18nc("Mouse action", "Double-click")), 0, 1);
        helpBoxGrid->addWidget(new QLabel(i18nc("Keyboard action", "Enter")), 1, 1);

        helpBoxGrid->addWidget(new QLabel(i18n("Create new selection rectangle:")), 2, 0, Qt::AlignRight);
        helpBoxGrid->addWidget(new QLabel(i18nc("Mouse action", "Drag outside selection rectangle")), 2, 1);

        helpBoxGrid->addWidget(new QLabel(i18n("Move selection rectangle:")), 3, 0, Qt::AlignRight);
        helpBoxGrid->addWidget(new QLabel(i18nc("Mouse action", "Drag inside selection rectangle")), 3, 1);
        helpBoxGrid->addWidget(new QLabel(i18nc("Keyboard action", "Arrow keys")), 4, 1);
        helpBoxGrid->addWidget(new QLabel(i18nc("Keyboard action", "+ Shift: Move in 1 pixel steps")), 5, 1);

        helpBoxGrid->addWidget(new QLabel(i18n("Resize selection rectangle:")), 6, 0, Qt::AlignRight);
        helpBoxGrid->addWidget(new QLabel(i18nc("Mouse action", "Drag handles")), 6, 1);
        helpBoxGrid->addWidget(new QLabel(i18nc("Keyboard action", "Arrow keys + Alt")), 7, 1);
        helpBoxGrid->addWidget(new QLabel(i18nc("Keyboard action", "+ Shift: Resize in 1 pixel steps")), 8, 1);

        helpBoxGrid->addWidget(new QLabel(i18n("Reset selection:")), 9, 0, Qt::AlignRight);
        helpBoxGrid->addWidget(new QLabel(i18nc("Mouse action", "Right-click")), 9, 1);

        helpBoxGrid->addWidget(new QLabel(i18n("Cancel:")), 10, 0, Qt::AlignRight);
        helpBoxGrid->addWidget(new QLabel(i18nc("Keyboard action", "Escape")), 10, 1);
    }

    const QScreen *primaryScreen = QGuiApplication::primaryScreen();
    helpBox->resize(helpBoxGrid->sizeHint());
    helpBox->move(primaryScreen->geometry().x() + (primaryScreen->geometry().width() - helpBox->width()) / 2,
                  primaryScreen->geometry().y() + (primaryScreen->geometry().height() - helpBox->height()));

    auto updateHelpBoxVisibility = [this, helpBox]() {
        const QRectF selection = mSelectorItem->selection().normalized();
        helpBox->setVisible((Settings::useReleaseToCapture() && selection.isEmpty())
                            || (!selection.isEmpty() && !QRectF(helpBox->geometry()).intersects(selection)));
    };
    updateHelpBoxVisibility();
    connect(mSelectorItem, &AreaSelectorItem::selectionChanged, helpBox, updateHelpBoxVisibility);
    mScene->addWidget(helpBox);

    // Focus the area selector item as it handles keyboard input.
    mScene->setFocusItem(mSelectorItem);
}

QuickEditor::~QuickEditor()
{
    qDeleteAll(mViews);
    mViews.clear();

    delete mScene;
    mScene = nullptr;
}

void QuickEditor::captureSelection()
{
    // On Wayland, if screens have mismatching scale factors, the snapshot will have the
    // highest scale factor. On X11, the snapshot will be provided in the device pixels, i.e.
    // no upscaling will occur.

    const QRect selection = mSelectorItem->selection().toRect();
    QImage image;
    if (KWindowSystem::isPlatformWayland()) {
        image = captureSelectionWayland(selection);
    } else if (KWindowSystem::isPlatformX11()) {
        image = captureSelectionX11(selection);
    }
    Q_EMIT grabDone(QPixmap::fromImage(image));
}

QImage QuickEditor::captureSelectionX11(const QRect &selection)
{
    const QList<QScreen *> screens = QGuiApplication::screens();

    QHash<const QScreen *, QRect> parts;
    parts.reserve(screens.size());
    for (const QScreen *screen : screens) {
        const QRect rect = selection & screen->geometry();
        if (rect.isEmpty()) {
            continue;
        }

        // Note that the screen's position on X11 is in the native pixels, while the
        // size is in the device-independent pixels.
        const QRectF scaledRect(screen->geometry().x() + (rect.x() - screen->geometry().x()) * screen->devicePixelRatio(),
                                screen->geometry().y() + (rect.y() - screen->geometry().y()) * screen->devicePixelRatio(),
                                rect.width() * screen->devicePixelRatio(),
                                rect.height() * screen->devicePixelRatio());
        parts.insert(screen, scaledRect.toAlignedRect());
    }

    const QRect viewport = std::accumulate(parts.constBegin(), parts.constEnd(), QRect(), [](auto a, auto b) {
        return a.united(b);
    });

    QImage result(viewport.size(), QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);

    // Not iterating over the part so the paint order is predefined.
    for (const QScreen *screen : screens) {
        const QRect part = parts.value(screen);
        if (!part.isValid()) {
            continue;
        }

        QRect localPart = part;
        localPart.translate(-screen->geometry().topLeft());

        QPainter painter(&result);
        painter.setWindow(viewport);
        painter.drawImage(part, mImages[screen], localPart);
    }

    return QImage();
}

QImage QuickEditor::captureSelectionWayland(const QRect &selection)
{
    qreal devicePixelRatio = 1;
    const QList<QScreen *> screens = QGuiApplication::screens();
    for (const QScreen *screen : screens) {
        if (screen->devicePixelRatio() > devicePixelRatio) {
            devicePixelRatio = screen->devicePixelRatio();
        }
    }

    QImage result(selection.size() * devicePixelRatio, QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);
    result.setDevicePixelRatio(devicePixelRatio);

    for (const QScreen *screen : screens) {
        const QRect targetRect = selection & screen->geometry();
        if (targetRect.isEmpty()) {
            continue;
        }
        const QImage source = mImages.value(screen);
        if (source.isNull()) {
            continue;
        }

        const QRectF sourceRect((targetRect.x() - screen->geometry().x()) * screen->devicePixelRatio(),
                                (targetRect.y() - screen->geometry().y()) * screen->devicePixelRatio(),
                                targetRect.width() * screen->devicePixelRatio(),
                                targetRect.height() * screen->devicePixelRatio());

        QPainter painter(&result);
        painter.translate(-selection.topLeft());
        painter.drawImage(targetRect, source, sourceRect);
    }

    return result;
}
