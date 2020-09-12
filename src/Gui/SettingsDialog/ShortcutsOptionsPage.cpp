/*
 * Copyright (C) 2019  David Redondo <kde@david-redondo.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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


