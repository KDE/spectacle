/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "CommandLineOptions.h"

struct CommandLineOptionsSingleton {
    CommandLineOptions self;
};

Q_GLOBAL_STATIC(CommandLineOptionsSingleton, privateCommandLineOptionsSelf)

CommandLineOptions *CommandLineOptions::self()
{
    return &privateCommandLineOptionsSelf()->self;
}
