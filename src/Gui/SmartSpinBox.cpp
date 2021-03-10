/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QtMath>
#include <KLocalizedString>

#include "SmartSpinBox.h"

SmartSpinBox::SmartSpinBox(QWidget *parent) :
    QDoubleSpinBox(parent)
{
    connect(this, qOverload<qreal>(&SmartSpinBox::valueChanged),
            this, &SmartSpinBox::suffixChangeHandler);
}

QString SmartSpinBox::textFromValue(double val) const
{
    if ((qFloor(val) == val) && (qCeil(val) == val)) {
        return QWidget::locale().toString(qint64(val));
    }
    return QWidget::locale().toString(val, 'f', decimals());
}

void SmartSpinBox::suffixChangeHandler(double val)
{
    int integerSeconds = static_cast<int>(val);
    if (val == integerSeconds) {
        setSuffix(i18ncp("Integer number of seconds", " second", " seconds", integerSeconds));
    } else {
        setSuffix(i18nc("Decimal number of seconds", " seconds"));
    }
}
