/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15

SavedMessage {
    text:i18n("The screenshot was copied to the clipboard and saved as <a href=\"%1\">%2</a>",
              messageArgument, contextWindow.baseFileName(messageArgument))
}
