// SPDX-FileCopyrightText: 2021 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once
#include <QObject>

class TextExtractor : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void doExtract(const QString &location);

Q_SIGNALS:
    void errorOccured(const QString &error);
};
