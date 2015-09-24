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

#ifndef SPECTACLEDBUSADAPTER_H
#define SPECTACLEDBUSADAPTER_H

#include <QDBusAbstractAdaptor>
#include "SpectacleCore.h"

class SpectacleDBusAdapter: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Screenshot")
    Q_CLASSINFO("D-Bus Introspection", ""
        "  <interface name=\"org.freedesktop.Screenshot\">\n"
        "    <method name=\"StartAgent\">\n"
        "    </method>\n"
        "    <method name=\"FullScreen\">\n"
        "      <arg direction=\"in\" type=\"b\" name=\"includeMousePointer\"/>\n"
        "    </method>\n"
        "    <method name=\"CurrentScreen\">\n"
        "      <arg direction=\"in\" type=\"b\" name=\"includeMousePointer\"/>\n"
        "    </method>\n"
        "    <method name=\"ActiveWindow\">\n"
        "      <arg direction=\"in\" type=\"b\" name=\"includeWindowDecorations\"/>\n"
        "      <arg direction=\"in\" type=\"b\" name=\"includeMousePointer\"/>\n"
        "    </method>\n"
        "    <method name=\"WindowUnderCursor\">\n"
        "      <arg direction=\"in\" type=\"b\" name=\"includeWindowDecorations\"/>\n"
        "      <arg direction=\"in\" type=\"b\" name=\"includeMousePointer\"/>\n"
        "    </method>\n"
        "    <signal name=\"ScreenshotTaken\">\n"
        "      <arg direction=\"out\" type=\"s\" name=\"fileName\"/>\n"
        "    </signal>\n"
        "    <signal name=\"ScreenshotFailed\">\n"
        "    </signal>\n"
        "  </interface>\n"
        ""
    )

    public:

    SpectacleDBusAdapter(SpectacleCore *parent);
    virtual ~SpectacleDBusAdapter();

    inline SpectacleCore *parent() const;

    public slots:

    Q_NOREPLY void StartAgent();
    Q_NOREPLY void FullScreen(bool includeMousePointer);
    Q_NOREPLY void CurrentScreen(bool includeMousePointer);
    Q_NOREPLY void ActiveWindow(bool includeWindowDecorations, bool includeMousePointer);
    Q_NOREPLY void WindowUnderCursor(bool includeWindowDecorations, bool includeMousePointer);

    signals:

    void ScreenshotTaken(const QString &fileName);
    void ScreenshotFailed();
};

#endif // SPECTACLEDBUSADAPTER_H
