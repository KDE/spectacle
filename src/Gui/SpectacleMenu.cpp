/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SpectacleMenu.h"
#include "WidgetWindowUtils.h"
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>

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
    auto point = item->mapToGlobal({0, item->height()});
    auto screenRect = itemWindow->screen()->geometry();
    auto sizeHint = this->sizeHint();
    if (point.y() + sizeHint.height() > screenRect.bottom()) {
        point.setY(point.y() - item->height() - sizeHint.height());
    }
    if (point.x() + sizeHint.width() > screenRect.right()) {
        point.setX(point.x() - sizeHint.width() + item->width());
    }
    setWidgetTransientParent(this, itemWindow);
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
