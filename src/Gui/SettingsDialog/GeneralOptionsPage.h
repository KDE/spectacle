/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef GENERALOPTIONSPAGE_H
#define GENERALOPTIONSPAGE_H

#include <QScopedPointer>
#include <QWidget>

class Ui_GeneralOptions;

class GeneralOptionsPage : public QWidget
{
    Q_OBJECT

    public:

    explicit GeneralOptionsPage(QWidget *parent = nullptr);
    ~GeneralOptionsPage() override;

    private:

    void updateAutomaticActions();

    QScopedPointer<Ui_GeneralOptions> m_ui;
};

#endif // GENERALOPTIONSPAGE_H
