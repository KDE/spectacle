/*
 *  Copyright (C) 2015 Boudhayan Gupta <me@BaloneyGeek.com>
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

#ifndef KSSAVECONFIGDIALOG_H
#define KSSAVECONFIGDIALOG_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QLabel>
#include <QFont>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStandardPaths>

#include <KLocalizedString>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KIOWidgets/KUrlRequester>


class KSSaveConfigDialog : public QDialog
{
    Q_OBJECT

    public:

    explicit KSSaveConfigDialog(QWidget *parent = 0);
    ~KSSaveConfigDialog();

    public slots:

    void accept() Q_DECL_OVERRIDE;

    private:

    QDialogButtonBox *mDialogButtonBox;
    KUrlRequester    *mUrlRequester;
    QLineEdit        *mSaveNameFormat;
};

#endif // KSSAVECONFIGDIALOG_H
