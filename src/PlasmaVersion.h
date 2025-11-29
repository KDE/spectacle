/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QtGlobal>

class PlasmaVersion
{
public:
    /**
     * Get the plasma version as an unsigned int.
     */
    static quint32 get();

    /**
     * Use this for plasma versions the same way you'd use QT_VERSION_CHECK()
     */
    static quint32 check(quint8 major, quint8 minor, quint8 patch);

private:
    PlasmaVersion() = delete;
    ~PlasmaVersion() = delete;
    PlasmaVersion(const PlasmaVersion &) = delete;
    PlasmaVersion(PlasmaVersion &&) = delete;
    PlasmaVersion &operator=(const PlasmaVersion &) = delete;
    PlasmaVersion &operator=(PlasmaVersion &&) = delete;
};
