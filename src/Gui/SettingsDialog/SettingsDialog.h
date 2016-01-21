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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QSet>
#include <KPageDialog>

class KPageWidgetItem;

class SettingsDialog : public KPageDialog
{
    Q_OBJECT

    public:

    explicit SettingsDialog(QWidget *parent = 0);
    virtual ~SettingsDialog();

    public slots:

    void accept() Q_DECL_OVERRIDE;

    private slots:

    void initPages();

    private:

    QSet<KPageWidgetItem *> mPages;
};

#endif // SETTINGSDIALOG_H
