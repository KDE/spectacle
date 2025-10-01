/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <KLocalizedString>
#include <QCommandLineOption>
#include <QList>

using namespace Qt::StringLiterals;

struct CommandLineOptions {
    static CommandLineOptions *self();
    static QString toArgument(const QCommandLineOption &option) {
        return u"--" + option.names().constLast();
    }
    // i18n() can't be used in static code,
    // so we can't just make the variables static and use them directly.
    const QCommandLineOption fullscreen = {
        {u"f"_s, u"fullscreen"_s},
        i18n("Capture the entire desktop (default)")
    };
    const QCommandLineOption current = {
        {u"m"_s, u"current"_s},
        i18n("Capture the current monitor")
    };
    const QCommandLineOption activeWindow = {
        {u"a"_s, u"activewindow"_s},
        i18n("Capture the active window")
    };
    const QCommandLineOption windowUnderCursor = {
        {u"u"_s, u"windowundercursor"_s},
        i18n("Capture the window currently under the cursor, including parents of pop-up menus")
    };
    const QCommandLineOption transientOnly = {
        {u"t"_s, u"transientonly"_s},
        i18n("Capture the window currently under the cursor, excluding parents of pop-up menus")
    };
    const QCommandLineOption region = {
        {u"r"_s, u"region"_s},
        i18n("Capture a rectangular region of the screen")
    };
    const QCommandLineOption record = {
        {u"R"_s, u"record"_s},
        i18n("Record the screen using the given mode. Modes:\n"
             "- r, region\n"
             "- s, screen\n"
             "- w, window"),
        u"mode"_s,
    };
    const QCommandLineOption launchOnly = {
        {u"l"_s, u"launchonly"_s},
        i18n("Launch Spectacle without taking a screenshot")
    };
    const QCommandLineOption gui = {
        {u"g"_s, u"gui"_s},
        i18n("Start in GUI mode (default)")
    };
    const QCommandLineOption background = {
        {u"b"_s, u"background"_s},
        i18n("Take a screenshot and exit without showing the GUI")
    };
    const QCommandLineOption dbus = {
        {u"s"_s, u"dbus"_s},
        i18n("Start in DBus-Activation mode")
    };
    const QCommandLineOption noNotify = {
        {u"n"_s, u"nonotify"_s},
        i18n("In background mode, do not pop up a notification when the screenshot is taken")
    };
    const QCommandLineOption output = {
        {u"o"_s, u"output"_s},
        i18n("In background mode, save image to specified file"),
        u"fileName"_s
    };
    const QCommandLineOption delay = {
        {u"d"_s, u"delay"_s},
        i18n("In background mode, delay before taking the shot (in milliseconds)"),
        u"delayMsec"_s
    };
    const QCommandLineOption copyImage = {
        {u"c"_s, u"copy-image"_s},
        i18n("In background mode, copy screenshot image to clipboard, unless -o is also used.")
    };
    const QCommandLineOption copyPath = {
        {u"C"_s, u"copy-path"_s},
        i18n("In background mode, copy screenshot file path to clipboard")
    };
    const QCommandLineOption onClick = {
        {u"w"_s, u"onclick"_s},
        i18n("Wait for a click before taking screenshot. Invalidates delay")
    };
    const QCommandLineOption newInstance = {
        {u"i"_s, u"new-instance"_s},
        i18n("Starts a new GUI instance of spectacle without registering to DBus")
    };
    const QCommandLineOption pointer = {
        {u"p"_s, u"pointer"_s},
        i18n("In background mode, include pointer in the screenshot")
    };
    const QCommandLineOption noDecoration = {
        {u"e"_s, u"no-decoration"_s},
        i18n("In background mode, exclude decorations in the screenshot")
    };
    const QCommandLineOption noShadow = {{u"S"_s, u"no-shadow"_s}, i18n("In background mode, exclude shadows in the screenshot")};
    const QCommandLineOption editExisting = {
        {u"E"_s, u"edit-existing"_s},
        i18n("Open and edit existing screenshot file"),
        u"existingFileName"_s
    };
    const QCommandLineOption releaseCapture = {{u"k"_s, u"release-capture"_s}, i18n("Accept region selection on click-and-release")};

    const QList<QCommandLineOption> allOptions = {
        fullscreen, current,     activeWindow, windowUnderCursor, transientOnly, region,       record,         launchOnly,
        gui,        background,  dbus,         noNotify,          output,        delay,        copyImage,      copyPath,
        onClick,    newInstance, pointer,      noDecoration,      noShadow,      editExisting, releaseCapture,
    };

    // Keep order in sync with allOptions
    enum Option {
        Fullscreen,
        Current,
        ActiveWindow,
        WindowUnderCursor,
        TransientOnly,
        Region,
        Record,
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
        NoShadow,
        EditExisting,
        ReleaseCapture,
        TotalOptions
    };
};
