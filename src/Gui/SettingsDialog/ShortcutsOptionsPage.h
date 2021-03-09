/*
 * SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SHORTCUTSOPTIONSPAGE_H
#define SHORTCUTSOPTIONSPAGE_H

#include <QWidget>

class KShortcutsEditor;

class ShortcutsOptionsPage : public QWidget
{
    Q_OBJECT

    public:

    explicit ShortcutsOptionsPage (QWidget* parent);
    ~ShortcutsOptionsPage();

    bool isModified();
    void defaults();

    Q_SIGNALS:
    void shortCutsChanged();

    public Q_SLOTS:

    void saveChanges();
    void resetChanges();


    private:

    KShortcutsEditor* mEditor;
};

#endif // SHORTCUTSOPTIONSPAGE_H
