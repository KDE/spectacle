/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QWidget>
#include <QWindow>

/* A small collection of functions to set the transient parents of QWidgets without segfaults.
 * For some reason, we have to check winId() to avoid crashes.
 */

inline void setWidgetTransientParent(QWidget *widget, QWindow *parent)
{
    if (widget && widget->winId() && parent && parent->winId()) {
        widget->windowHandle()->setTransientParent(parent);
    }
}

inline void setWidgetTransientParentToWidget(QWidget *widget, QWidget *parent)
{
    if (widget && widget->winId() && parent && parent->winId()) {
        widget->windowHandle()->setTransientParent(parent->windowHandle());
    }
}

inline QWindow *getWidgetTransientParent(QWidget *widget)
{
    return widget && widget->winId() ? widget->windowHandle()->transientParent() : nullptr;
}
