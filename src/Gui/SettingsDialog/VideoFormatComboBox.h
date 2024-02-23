/* SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "VideoFormatModel.h"
#include <QComboBox>

class VideoFormatComboBox : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(VideoPlatform::Format currentFormat READ currentFormat WRITE setCurrentFormat NOTIFY currentFormatChanged)
public:
    VideoFormatComboBox(VideoFormatModel *model, QWidget *parent = nullptr);
    VideoPlatform::Format currentFormat() const;
    void setCurrentFormat(VideoPlatform::Format format);
    Q_SIGNAL void currentFormatChanged();
};
