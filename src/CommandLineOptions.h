/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <KLocalizedString>
#include <QCommandLineOption>
#include <QList>

struct CommandLineOptions {
    static CommandLineOptions *self();
    static QString toArgument(const QCommandLineOption &option) {
        return QStringLiteral("--") + option.names().constLast();
    }
    // i18n() can't be used in static code,
    // so we can't just make the variables static and use them directly.
    const QCommandLineOption fullscreen = {
        {QStringLiteral("f"), QStringLiteral("fullscreen")},
        i18n("Capture the entire desktop (default)")
    };
    const QCommandLineOption current = {
        {QStringLiteral("m"), QStringLiteral("current")},
        i18n("Capture the current monitor")
    };
    const QCommandLineOption activeWindow = {
        {QStringLiteral("a"), QStringLiteral("activewindow")},
        i18n("Capture the active window")
    };
    const QCommandLineOption windowUnderCursor = {
        {QStringLiteral("u"), QStringLiteral("windowundercursor")},
        i18n("Capture the window currently under the cursor, including parents of pop-up menus")
    };
    const QCommandLineOption transientOnly = {
        {QStringLiteral("t"), QStringLiteral("transientonly")},
        i18n("Capture the window currently under the cursor, excluding parents of pop-up menus")
    };
    const QCommandLineOption region = {
        {QStringLiteral("r"), QStringLiteral("region")},
        i18n("Capture a rectangular region of the screen")
    };
    const QCommandLineOption launchOnly = {
        {QStringLiteral("l"), QStringLiteral("launchonly")},
        i18n("Launch Spectacle without taking a screenshot")
    };
    const QCommandLineOption gui = {
        {QStringLiteral("g"), QStringLiteral("gui")},
        i18n("Start in GUI mode (default)")
    };
    const QCommandLineOption background = {
        {QStringLiteral("b"), QStringLiteral("background")},
        i18n("Take a screenshot and exit without showing the GUI")
    };
    const QCommandLineOption dbus = {
        {QStringLiteral("s"), QStringLiteral("dbus")},
        i18n("Start in DBus-Activation mode")
    };
    const QCommandLineOption noNotify = {
        {QStringLiteral("n"), QStringLiteral("nonotify")},
        i18n("In background mode, do not pop up a notification when the screenshot is taken")
    };
    const QCommandLineOption output = {
        {QStringLiteral("o"), QStringLiteral("output")},
        i18n("In background mode, save image to specified file"),
        QStringLiteral("fileName")
    };
    const QCommandLineOption delay = {
        {QStringLiteral("d"), QStringLiteral("delay")},
        i18n("In background mode, delay before taking the shot (in milliseconds)"),
        QStringLiteral("delayMsec")
    };
    const QCommandLineOption copyImage = {
        {QStringLiteral("c"), QStringLiteral("copy-image")},
        i18n("In background mode, copy screenshot image to clipboard, unless -o is also used.")
    };
    const QCommandLineOption copyPath = {
        {QStringLiteral("C"), QStringLiteral("copy-path")},
        i18n("In background mode, copy screenshot file path to clipboard")
    };
    const QCommandLineOption onClick = {
        {QStringLiteral("w"), QStringLiteral("onclick")},
        i18n("Wait for a click before taking screenshot. Invalidates delay")
    };
    const QCommandLineOption newInstance = {
        {QStringLiteral("i"), QStringLiteral("new-instance")},
        i18n("Starts a new GUI instance of spectacle without registering to DBus")
    };
    const QCommandLineOption pointer = {
        {QStringLiteral("p"), QStringLiteral("pointer")},
        i18n("In background mode, include pointer in the screenshot")
    };
    const QCommandLineOption noDecoration = {
        {QStringLiteral("e"), QStringLiteral("no-decoration")},
        i18n("In background mode, exclude decorations in the screenshot")
    };
    const QCommandLineOption editExisting = {
        {QStringLiteral("E"), QStringLiteral("edit-existing")},
        i18n("Open and edit existing screenshot file"),
        QStringLiteral("existingFileName")
    };

    const QList<QCommandLineOption> allOptions = {
        fullscreen,
        current,
        activeWindow,
        windowUnderCursor,
        transientOnly,
        region,
        launchOnly,
        gui,
        background,
        dbus,
        noNotify,
        output,
        delay,
        copyImage,
        copyPath,
        onClick,
        newInstance,
        pointer,
        noDecoration,
        editExisting
    };

    // Keep order in sync with allOptions
    enum Option {
        Fullscreen,
        Current,
        ActiveWindow,
        WindowUnderCursor,
        TransientOnly,
        Region,
        LaunchOnly,
        Gui,
        Background,
        DBus,
        NoNotify,
        Output,
        Delay,
        CopyImage,
        CopyPath,
        OnClick,
        NewInstance,
        Pointer,
        NoDecoration,
        EditExisting,
        TotalOptions
    };
};
