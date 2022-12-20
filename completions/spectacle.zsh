#compdef spectacle

# SPDX-FileCopyrightText: 2022 ivan tkachenko <me@ratijas.tk>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Note: '+' argument denotes groupping, while '-' is for mutually exclusive sets of options

_arguments -s \
  '(- *)'{-h,--help}'[Displays help on commandline options]' \
  '(- *)'{-v,--version}'[Displays version information]' \
  \
  '(-i --new-instance)'{-i,--new-instance}'[Starts a new GUI instance of spectacle without registering to DBus]' \
  '(-E --edit-existing)'{-E=,--edit-existing=}'[Open and edit existing screenshot file]::existingFileName:_files' \
  \
  + capture-mode \
    '(capture-mode)'{-f,--fullscreen}'[Capture the entire desktop (default)]' \
    '(capture-mode)'{-m,--current}'[Capture the current monitor]' \
    '(capture-mode)'{-a,--activewindow}'[Capture the active window]' \
    '(capture-mode)'{-r,--region}'[Capture a rectangular region of the screen]' \
    '(capture-mode)'{-u,--windowundercursor}'[Capture the window currently under the cursor, including parents of pop-up menus]' \
    '(capture-mode)'{-t,--transientonly}'[Capture the window currently under the cursor, excluding parents of pop-up menus]' \
    '(capture-mode)'{-l,--launchonly}'[Launch Spectacle without taking a screenshot]' \
  - dbus \
    '(-s --dbus)'{-s,--dbus}'[Start in DBus-Activation mode]' \
  - gui \
    '(-g --gui)'{-g,--gui}'[Start in GUI mode (default)]' \
  - background \
    '(-b --background)'{-b,--background}'[Take a screenshot and exit without showing the GUI]' \
    '(-n --nonotify)'{-n,--nonotify}'[In background mode, do not pop up a notification when the screenshot is taken]' \
    '(-c --copy-image -C --copy-path)'{-c,--copy-image}'[In background mode, copy screenshot image to clipboard, unless -o is also used.]' \
    '(-c --copy-image -C --copy-path)'{-C,--copy-path}'[In background mode, copy screenshot file path to clipboard]' \
    '(-d --delay)'{-d=,--delay=}'[In background mode, delay before taking the shot (in milliseconds)]::delayMsec:_numbers "milliseconds"' \
    '(-w --onclick)'{-w,--onclick}'[Wait for a click before taking screenshot. Invalidates delay]' \
    '(-p --pointer)'{-p,--pointer}'[In background mode, include pointer in the screenshot]' \
    '(-e --no-decoration)'{-e,--no-decoration}'[In background mode, exclude decorations in the screenshot]' \
    '(-o --output)'{-o=,--output=}'[In background mode, save image to specified file]::output file:_files' \
