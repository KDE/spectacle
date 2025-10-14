/* SPDX-FileCopyrightText: 2025 Jhair Paris <dev@jhairparis.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick.Templates as T
import org.kde.spectacle.private

T.Action {
    enabled: !SpectacleCore.videoMode && 
             SpectacleCore.ocrAvailable && 
             SpectacleCore.ocrStatus !== 1
    icon.name: "document-scan"
    text: i18nc("@action", "Extract Text")
    onTriggered: SpectacleCore.startOcrExtraction()
}
