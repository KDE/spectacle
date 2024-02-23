/* SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "VideoFormatComboBox.h"

using namespace Qt::StringLiterals;

VideoFormatComboBox::VideoFormatComboBox(VideoFormatModel *model, QWidget *parent)
    : QComboBox(parent)
{
    setModel(model);
    setObjectName(u"kcfg_preferredVideoFormat"_s);
    setProperty("kcfg_property", u"currentFormat"_s);
    connect(this, &QComboBox::currentIndexChanged, this, &VideoFormatComboBox::currentFormatChanged);
}

VideoPlatform::Format VideoFormatComboBox::currentFormat() const
{
    return currentData(VideoFormatModel::FormatRole).value<VideoPlatform::Format>();
}

void VideoFormatComboBox::setCurrentFormat(VideoPlatform::Format format)
{
    auto model = static_cast<VideoFormatModel *>(this->model());
    setCurrentIndex(model->indexOfFormat(format));
}

#include "moc_VideoFormatComboBox.cpp"
