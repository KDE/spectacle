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

#include <QScopedPointer>
#include <QWidget>

class Ui_SaveOptions;

class SaveOptionsPage : public QWidget
{
    Q_OBJECT

    public:

    explicit SaveOptionsPage(QWidget *parent = nullptr);
    ~SaveOptionsPage() override;

    private:

    QScopedPointer<Ui_SaveOptions> m_ui;

    void updateFilenamePreview();
};

#endif // SAVEOPTIONSPAGE_H
