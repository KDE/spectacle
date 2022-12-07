/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SpectacleMenu.h"
#include <QQuickItem>
#include <QQuickWindow>
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
