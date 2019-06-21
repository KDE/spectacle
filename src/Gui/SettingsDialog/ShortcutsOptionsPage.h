#ifndef SHORTCUTSOPTIONSPAGE_H
#define SHORTCUTSOPTIONSPAGE_H

#include "SettingsPage.h"

class KShortcutsEditor;

class ShortcutsOptionsPage : public SettingsPage
{
    Q_OBJECT

    public:

    explicit ShortcutsOptionsPage ( QWidget* parent );
    ~ShortcutsOptionsPage();

    public Q_SLOTS:

    void saveChanges() override;
    void resetChanges() override;

    private Q_SLOTS:

    void markDirty();

    private:

    KShortcutsEditor* mEditor;
};

#endif // SHORTCUTSOPTIONSPAGE_H
