/*
 * SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ShortcutsOptionsPage.h"

#include "ShortcutActions.h"

#include <KShortcutsEditor>

#include <QVBoxLayout>

ShortcutsOptionsPage::ShortcutsOptionsPage(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);
    mEditor = new KShortcutsEditor(ShortcutActions::self()->shortcutActions(), this, KShortcutsEditor::ActionType::GlobalAction);
    mainLayout->addWidget(mEditor);
    connect(mEditor, &KShortcutsEditor::keyChange, this, &ShortcutsOptionsPage::shortCutsChanged);
}

ShortcutsOptionsPage::~ShortcutsOptionsPage()
{
    mEditor->undo();
}

void ShortcutsOptionsPage::resetChanges()
{
    mEditor->undo();
}

void ShortcutsOptionsPage::saveChanges()
{
    mEditor->commit();
}

bool ShortcutsOptionsPage::isModified() const
{
    return mEditor->isModified();
}

void ShortcutsOptionsPage::defaults() const
{
    mEditor->allDefault();
}
