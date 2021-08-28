/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
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
