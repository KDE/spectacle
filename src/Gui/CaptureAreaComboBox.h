/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@zohomail.eu>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef CAPTUREAREACOMBOBOX_H
#define CAPTUREAREACOMBOBOX_H

#include "Platforms/Platform.h"

#include "SpectacleCommon.h"

#include <qcombobox.h>

class CaptureModeDelegate;

/**
 * @brief The most prominent ComboBox of the Spectacle UI.
 *
 * This class exists so keyboard shortcuts for the actions can be drawn in the popup.
 */
class CaptureAreaComboBox : public QComboBox
{
public:
    /**
     * @param grabModes The screenshotting modes which the current platform supports.
     */
    explicit CaptureAreaComboBox(Platform::GrabModes grabModes, QWidget *parent);

    inline Spectacle::CaptureMode currentCaptureMode() const
    {
        return static_cast<Spectacle::CaptureMode>(currentData().toInt());
    };
    inline Spectacle::CaptureMode captureModeForIndex(int index) const
    {
        return static_cast<Spectacle::CaptureMode>(itemData(index).toInt());
    };

protected:
    /**
     * Determines the current keyboard shortcuts to be displayed and the necessary width of the popup before calling the base method QComboBox::showPopup().
     */
    void showPopup() override;

private:
    /** @param grabModes The screenshotting modes which the current platform supports. */
    Platform::GrabModes mGrabModes;

    /** The QComboBox::itemDelegate() of this object. */
    CaptureModeDelegate *mCaptureModeDelegate;
};

#endif // CAPTUREAREACOMBOBOX_H
