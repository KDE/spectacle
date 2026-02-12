/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef SMARTSPINBOX_H
#define SMARTSPINBOX_H

#include <QDoubleSpinBox>

class SmartSpinBox : public QDoubleSpinBox
{
    Q_OBJECT

public:
    explicit SmartSpinBox(QWidget *parent = nullptr);
    QString textFromValue(double val) const override;
    void stepBy(int steps) override;
    const std::function<int(int)> &stepFunction() const;
    void setStepFunction(const std::function<int(int)> &lambda);

private Q_SLOTS:
    void suffixChangeHandler(double val);

private:
    std::function<int(int)> m_stepFunction;
};

#endif // SMARTSPINBOX_H
