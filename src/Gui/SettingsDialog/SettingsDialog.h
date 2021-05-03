/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <KConfigDialog>

class ShortcutsOptionsPage;

class SettingsDialog : public KConfigDialog
{
    Q_OBJECT

    public:

    explicit SettingsDialog(QWidget *parent = nullptr);

    protected:

    QSize sizeHint() const override;

    private:

    bool hasChanged() override;
    bool isDefault() override;
    void updateSettings() override;
    void updateWidgets() override;
    void updateWidgetsDefault() override;

    ShortcutsOptionsPage* mShortcutsPage;
};

#endif // SETTINGSDIALOG_H
