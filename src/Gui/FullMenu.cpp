/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "FullMenu.h"
#include "ExportMenu.h"
#include "HelpMenu.h"
#include "WidgetWindowUtils.h"
#include <KStandardActions>
#include <QStyle>

using namespace Qt::StringLiterals;

static QPointer<FullMenu> s_instance = nullptr;

FullMenu::FullMenu(QWidget *parent)
    : OptionsMenu(parent)
{
    connect(this, &FullMenu::aboutToShow,
            this, [this] {
                setWidgetTransientParentToWidget(HelpMenu::instance(), this);
            });

    addMenu(HelpMenu::instance());
}

FullMenu *FullMenu::instance()
{
    // We need to create it here instead of using Q_GLOBAL_STATIC like the other menus
    // to prevent a segfault.
    if (!s_instance) {
        s_instance = new FullMenu;
    }
    return s_instance;
}

#include "moc_FullMenu.cpp"
