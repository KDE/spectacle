/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QScopedPointer>
#include <QWidget>

class Ui_VideoSaveOptions;
class VideoFormatComboBox;

class VideoSaveOptionsPage : public QWidget
{
    Q_OBJECT

public:
    explicit VideoSaveOptionsPage(QWidget *parent = nullptr);
    ~VideoSaveOptionsPage() override;

private:
    QScopedPointer<Ui_VideoSaveOptions> m_ui;
    std::unique_ptr<VideoFormatComboBox> m_videoFormatComboBox;

    void updateFilenamePreview();
};
