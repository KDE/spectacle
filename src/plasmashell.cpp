/*
 *  SPDX-FileCopyrightText: 2023 Nicolas Fella <nicolas.fella@gmx.de>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "plasmashell.h"

#include "qwayland-plasma-shell.h"

PlasmaShell::PlasmaShell()
    : QWaylandClientExtensionTemplate<PlasmaShell>(8)
{
}

PlasmaShellSurface::PlasmaShellSurface(struct ::org_kde_plasma_surface *object, QObject *parent)
    : QObject(parent)
    , QtWayland::org_kde_plasma_surface(object)
{
}
