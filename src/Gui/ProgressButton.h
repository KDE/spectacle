/*
 * SPDX-FileCopyrightText: 2019 David Redondo <david@david-redondo.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef PROGRESSBUTTON_H
#define PROGRESSBUTTON_H

#include <QToolButton>

/**
 * @todo write docs
 */
class ProgressButton : public  QToolButton
{
    Q_OBJECT
public:

    explicit ProgressButton(QWidget* parent);

    void setProgress(double progress);

protected:
    void paintEvent(QPaintEvent* event) override;

    double mProgress = 0.0;
};

#endif // PROGRESSBUTTON_H
