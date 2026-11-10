/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SpectacleMenu.h"
#include "WidgetWindowUtils.h"
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>

#include <KWindowSystem>

SpectacleMenu::SpectacleMenu(const QString &title, QWidget *parent)
    : QMenu(title, parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
}

SpectacleMenu::SpectacleMenu(QWidget *parent)
    : QMenu(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
}

void SpectacleMenu::setVisible(bool visible)
{
    bool oldVisible = isVisible();
    if (oldVisible == visible) {
        return;
    }
    // Workaround for a bug where Qt Quick buttons always open the menu even when the menu is already open
    if (visible) {
        QMenu::setVisible(true);
    } else {
        QTimer::singleShot(200, this, [this] {
            QMenu::setVisible(false);
        });
    }
}

void SpectacleMenu::popup(QQuickItem *item)
{
    if (!item || !item->window()) {
        return;
    }

    auto itemWindow = item->window();
    setWidgetTransientParent(this, itemWindow);

    QPointF point;
    if (KWindowSystem::isPlatformWayland()) {
        const QRect anchorRect = QRectF{item->mapToScene(QPointF{0, 0}), item->size()}.toRect();
        Qt::Edges anchor = Qt::BottomEdge | Qt::LeftEdge;
        Qt::Edges gravity = Qt::BottomEdge | Qt::RightEdge;
        if (qApp->isRightToLeft()) {
            anchor = Qt::BottomEdge | Qt::RightEdge;
        }

        windowHandle()->setProperty("_q_waylandPopupAnchorRect", anchorRect);
        windowHandle()->setProperty("_q_waylandPopupAnchor", int(anchor));
        windowHandle()->setProperty("_q_waylandPopupGravity", int(gravity));
        windowHandle()->setProperty("_q_waylandPopupConstraintAdjustment", 1u | 8u); // slide_x | flip_y
    } else {
        // Ensure any lazy-loaded items are present when we manually position the popup.
        Q_EMIT aboutToShow();

        point = item->mapToGlobal({0, item->height()});
        auto screenRect = itemWindow->screen()->geometry();
        auto sizeHint = this->sizeHint();
        if (point.y() + sizeHint.height() > screenRect.bottom()) {
            point.setY(point.y() - item->height() - sizeHint.height());
        }
        if (point.x() + sizeHint.width() > screenRect.right()) {
            point.setX(point.x() - sizeHint.width() + item->width());
        }
    }

    // Workaround same as plasma to have click anywhereto close the menu
    QTimer::singleShot(0, this, [this, itemWindow, point]() {
        if (itemWindow->mouseGrabberItem()) {
            itemWindow->mouseGrabberItem()->ungrabMouse();
        }
        QMenu::popup(point.toPoint());
    });
}

void SpectacleMenu::showEvent(QShowEvent *event)
{
    QMenu::showEvent(event);
    Q_EMIT visibleChanged();
}

void SpectacleMenu::hideEvent(QHideEvent *event)
{
    QMenu::hideEvent(event);
    Q_EMIT visibleChanged();
}

#include "moc_SpectacleMenu.cpp"
