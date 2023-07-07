/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SpectacleMenu.h"
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
    QMenu::setVisible(visible);
    Q_EMIT visibleChanged();
}

void SpectacleMenu::popup(const QPointF &globalPos)
{
    // Workaround same as plasma to have click anywhereto close the menu
    QTimer::singleShot(0, this, [this, globalPos]() {
        QQuickWindow *tp = qobject_cast<QQuickWindow *>(windowHandle()->transientParent());
        if (tp->mouseGrabberItem()) {
            tp->mouseGrabberItem()->ungrabMouse();
        }
        QMenu::popup(globalPos.toPoint());
    });
}

void SpectacleMenu::popup(QQuickItem *item)
{
    if (!item) {
        return;
    }
    auto point = item->mapToGlobal({0, item->height()});
    auto screenRect = item->window()->screen()->geometry();
    auto sizeHint = this->sizeHint();
    if (point.y() + sizeHint.height() > screenRect.bottom()) {
        point.setY(point.y() - item->height() - sizeHint.height());
    }
    if (point.x() + sizeHint.width() > screenRect.right()) {
        point.setX(point.x() - sizeHint.width() + item->width());
    }
    popup(point);
}

#include "moc_SpectacleMenu.cpp"
