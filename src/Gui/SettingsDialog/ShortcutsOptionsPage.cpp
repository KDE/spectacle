#include "ShortcutsOptionsPage.h"

#include "SpectacleConfig.h"

#include <KShortcutsEditor>

#include <QVBoxLayout>

ShortcutsOptionsPage::ShortcutsOptionsPage(QWidget* parent) : SettingsPage(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);

    mEditor = new KShortcutsEditor(SpectacleConfig::instance()->shortCutActions, this, KShortcutsEditor::ActionType::GlobalAction);
    mainLayout->addWidget(mEditor);
    connect(mEditor, &KShortcutsEditor::keyChange, this, &ShortcutsOptionsPage::markDirty);
}

ShortcutsOptionsPage::~ShortcutsOptionsPage()
{
    mEditor->undoChanges();
}


void ShortcutsOptionsPage::resetChanges()
{
    mEditor->undoChanges();
    mChangesMade = false;
}

void ShortcutsOptionsPage::saveChanges()
{
    mEditor->commit();
    mChangesMade = false;
}

void ShortcutsOptionsPage::markDirty()
{
    mChangesMade = true;
}

