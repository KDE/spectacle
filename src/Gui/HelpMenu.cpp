/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "HelpMenu.h"

#include <KAboutData>

#include <QApplication>
#include <QDialog>
#include <QWindow>

#include <cstring>

class HelpMenuSingleton
{
public:
    HelpMenu self;
};

Q_GLOBAL_STATIC(HelpMenuSingleton, privateHelpMenuSelf)

static QObject *findWidgetOfType(const char *className)
{
    if (strlen(className) == 0) {
        return nullptr;
    }
    const auto widgets = qApp->allWidgets();
    for (const auto w : widgets) {
        if (w->inherits(className)) {
            return w;
        }
    }
    return nullptr;
}

HelpMenu::HelpMenu(QWidget* parent)
    : SpectacleMenu(parent)
    , kHelpMenu(new KHelpMenu(parent, KAboutData::applicationData(), true))
{
    addActions(kHelpMenu->menu()->actions());
    connect(this, &QMenu::triggered, this, &HelpMenu::onTriggered);
}

HelpMenu *HelpMenu::instance()
{
    return &privateHelpMenuSelf->self;
}

void HelpMenu::showAppHelp()
{
    kHelpMenu->appHelpActivated();
}

void HelpMenu::onTriggered(QAction *action)
{
    auto transientParent = windowHandle()->transientParent();
    if (!transientParent || !transientParent->isVisible() || action == kHelpMenu->action(KHelpMenu::menuWhatsThis)) {
        return;
    }

    QDialog *dialog = nullptr;
    // KHelpMenu creates these dialogs and sets the parent of KHelpMenu as the parent of the dialogs.
    // KHelpMenu doesn't expose pointers to the dialogs,
    // so we have to search for them in the parent.
    // 2 of the dialogs we need to find are private types.
    if (action == kHelpMenu->action(KHelpMenu::menuReportBug)) {
        dialog = qobject_cast<QDialog *>(findWidgetOfType("KBugReport"));
    } else if (action == kHelpMenu->action(KHelpMenu::menuSwitchLanguage)) {
        dialog = qobject_cast<QDialog *>(findWidgetOfType("KDEPrivate::KSwitchLanguageDialog"));
    } else if (action == kHelpMenu->action(KHelpMenu::menuAboutApp)) {
        dialog = qobject_cast<QDialog *>(findWidgetOfType("KAboutApplicationDialog"));
    } else if (action == kHelpMenu->action(KHelpMenu::menuAboutKDE)) {
        dialog = qobject_cast<QDialog *>(findWidgetOfType("KDEPrivate::KAboutKdeDialog"));
    }

    if (dialog) {
        if (dialog->winId()) {
            dialog->windowHandle()->setTransientParent(transientParent);
        }
        dialog->windowHandle()->requestActivate();
    }
}

#include "moc_HelpMenu.cpp"
