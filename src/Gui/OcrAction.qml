/* SPDX-FileCopyrightText: 2025 Jhair Paris <dev@jhairparis.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick.Templates as T
import org.kde.spectacle.private

T.Action {
    // OCR is only available for screenshots, not videos, and only when OCR is properly available
    enabled: !SpectacleCore.videoMode && SpectacleCore.ocrAvailable
    icon.name: "document-scan"
    text: i18nc("@action", "Extract Text")
    onTriggered: contextWindow.extractText()
}
