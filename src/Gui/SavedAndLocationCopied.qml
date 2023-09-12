/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick

SavedMessage {
    text: i18n("The screenshot has been saved as <a href=\"%1\">%2</a> and its location has been copied to clipboard",
               messageArgument, contextWindow.baseFileName(messageArgument))
}
