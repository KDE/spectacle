/*
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QScopedPointer>
#include <QWidget>

class Ui_VideoSaveOptions;
class AudioDeviceModel;
class VideoFormatComboBox;
class VideoFormatModel;

class VideoSaveOptionsPage : public QWidget
{
    Q_OBJECT

public:
    explicit VideoSaveOptionsPage(QWidget *parent = nullptr);
    ~VideoSaveOptionsPage() override;

    /// The audio devices chosen in the dialog, which are only saved when the settings are applied
    AudioDeviceModel *audioDeviceModel() const;

private:
    QScopedPointer<Ui_VideoSaveOptions> m_ui;
    std::unique_ptr<VideoFormatComboBox> m_videoFormatComboBox;
    std::unique_ptr<VideoFormatModel> m_videoFormatModel;
    AudioDeviceModel *const m_audioDeviceModel;

    void updateFilenamePreview();
    void updateAudioDevicesEnabled();
};
