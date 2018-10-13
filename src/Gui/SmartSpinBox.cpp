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

#include <QtMath>
#include <KLocalizedString>

#include "SmartSpinBox.h"

SmartSpinBox::SmartSpinBox(QWidget *parent) :
    QDoubleSpinBox(parent)
{
    connect(this, static_cast<void (SmartSpinBox::*)(qreal)>(&SmartSpinBox::valueChanged),
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
