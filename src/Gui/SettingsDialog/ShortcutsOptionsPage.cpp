/*
 * SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ShortcutsOptionsPage.h"

#include "ShortcutActions.h"

#include <KShortcutsEditor>
#include <kxmlgui_version.h>

#include <QVBoxLayout>

ShortcutsOptionsPage::ShortcutsOptionsPage(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);
    mEditor = new KShortcutsEditor(ShortcutActions::self()->shortcutActions(), this, KShortcutsEditor::ActionType::GlobalAction);
    mainLayout->addWidget(mEditor);
    connect(mEditor, &KShortcutsEditor::keyChange, this, &ShortcutsOptionsPage::shortCutsChanged);
}

ShortcutsOptionsPage::~ShortcutsOptionsPage()
{
#if KXMLGUI_VERSION >= QT_VERSION_CHECK(5, 75, 0)
    mEditor->undo();
#else
    mEditor->undoChanges();
#endif
}


void ShortcutsOptionsPage::resetChanges()
{
#if KXMLGUI_VERSION >= QT_VERSION_CHECK(5, 75, 0)
    mEditor->undo();
#else
    mEditor->undoChanges();
#endif
}

void ShortcutsOptionsPage::saveChanges()
{
    mEditor->commit();
}

bool ShortcutsOptionsPage::isModified()
{
    return mEditor->isModified();
}

void ShortcutsOptionsPage::defaults()
{
    mEditor->allDefault();
}


