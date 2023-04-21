/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QMenu>

class QQuickItem;

/**
 * This class only exists to make QMenu more usable with Qt Quick
 */
class SpectacleMenu : public QMenu
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged FINAL)
public:
    SpectacleMenu(const QString &title, QWidget *parent = nullptr);
    SpectacleMenu(QWidget *parent = nullptr);

    /**
     * Same as QMenu::setVisible, but it emits visibleChanged so it can be useful in QML bindings
     */
    void setVisible(bool visible) override;

    /**
     * Popup on the specified item
     */
    Q_INVOKABLE virtual void popup(QQuickItem *item);

Q_SIGNALS:
    void visibleChanged();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *) override;
};
