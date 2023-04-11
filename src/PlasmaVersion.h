/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QDebug>
#include <array>
#include <cstdint>

class PlasmaVersion
{
public:
    /**
     * Get the plasma version as an unsigned int.
     */
    static uint get();

    /**
     * Use this for plasama versions the same way you'd use QT_VERSION_CHECK()
     */
    static uint check(uchar major, uchar minor, uchar patch);

private:
    PlasmaVersion() = delete;
    ~PlasmaVersion() = delete;
    PlasmaVersion(const PlasmaVersion &) = delete;
    PlasmaVersion(PlasmaVersion &&) = delete;
    PlasmaVersion &operator=(const PlasmaVersion &) = delete;
    PlasmaVersion &operator=(PlasmaVersion &&) = delete;

    static uint s_plasmaVersion;
};
