/*
 *  SPDX-FileCopyrightText: 2023 Nicolas Fella <nicolas.fella@gmx.de>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "qwayland-plasma-shell.h"
#include <QtWaylandClient/qwaylandclientextension.h>

class PlasmaShell : public QWaylandClientExtensionTemplate<PlasmaShell>, public QtWayland::org_kde_plasma_shell
{
public:
    PlasmaShell();
};

class PlasmaShellSurface : public QObject, public QtWayland::org_kde_plasma_surface
{
public:
    PlasmaShellSurface(struct ::org_kde_plasma_surface *object, QObject *parent);
};
