/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef SAVEOPTIONSPAGE_H
#define SAVEOPTIONSPAGE_H

#include <QScopedPointer>
#include <QWidget>

class Ui_ImageSaveOptions;

class ImageSaveOptionsPage : public QWidget
{
    Q_OBJECT

public:
    explicit ImageSaveOptionsPage(QWidget *parent = nullptr);
    ~ImageSaveOptionsPage() override;

private:
    QScopedPointer<Ui_ImageSaveOptions> m_ui;

    void updateFilenamePreview();
};

#endif // SAVEOPTIONSPAGE_H
