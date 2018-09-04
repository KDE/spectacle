/*
 *  Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef SAVEOPTIONSPAGE_H
#define SAVEOPTIONSPAGE_H

#include "SettingsPage.h"

class QDialogButtonBox;
class QLineEdit;
class QComboBox;
class KUrlRequester;

class SaveOptionsPage : public SettingsPage
{
    Q_OBJECT

    public:

    explicit SaveOptionsPage(QWidget *parent = nullptr);

    public slots:

    void saveChanges() Q_DECL_OVERRIDE;
    void resetChanges() Q_DECL_OVERRIDE;

    private slots:

    void markDirty(const QString &text);

    private:

    QDialogButtonBox *mDialogButtonBox;
    QLineEdit        *mSaveNameFormat;
    KUrlRequester    *mUrlRequester;
    QComboBox        *mSaveImageFormat;
};

#endif // SAVEOPTIONSPAGE_H
