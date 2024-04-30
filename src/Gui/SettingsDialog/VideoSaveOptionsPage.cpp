/*
 *  SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "VideoSaveOptionsPage.h"

#include "Platforms/VideoPlatform.h"
#include "SpectacleCore.h"
#include "ExportManager.h"
#include "SaveOptionsUtils.h"
#include "VideoFormatComboBox.h"
#include "VideoFormatModel.h"
#include "ui_VideoSaveOptions.h"

#include <KLocalizedString>

#include <QCheckBox>
#include <QComboBox>
#include <QFontDatabase>
#include <QImageWriter>
#include <QLabel>
#include <QLineEdit>

using namespace Qt::StringLiterals;

VideoSaveOptionsPage::VideoSaveOptionsPage(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui_VideoSaveOptions)
{
    m_ui->setupUi(this);

    m_ui->preview->setFixedHeight(m_ui->kcfg_videoFilenameTemplate->height());

    const auto videoFormatModel = SpectacleCore::instance()->videoFormatModel();
    m_videoFormatComboBox = std::make_unique<VideoFormatComboBox>(videoFormatModel, this);
    m_ui->saveLayout->addWidget(m_videoFormatComboBox.get());

    // Auto select the correct format if the user types an extension in the filename template.
    connect(m_ui->kcfg_videoFilenameTemplate, &QLineEdit::textEdited, this, [this, videoFormatModel](const QString &text) {
        const auto count = videoFormatModel->rowCount();
        for (auto i = 0; i < count; ++i) {
            auto index = videoFormatModel->index(i);
            auto extension = index.data(VideoFormatModel::ExtensionRole).toString();
            if (text.endsWith(u'.' + extension, Qt::CaseInsensitive)) {
                m_ui->kcfg_videoFilenameTemplate->setText(text.chopped(extension.length() + 1));
                m_videoFormatComboBox->setCurrentIndex(i);
            }
        }
    });
    connect(m_ui->kcfg_videoFilenameTemplate, &QLineEdit::textChanged,
            this, &VideoSaveOptionsPage::updateFilenamePreview);
    connect(m_videoFormatComboBox.get(), &QComboBox::currentTextChanged, this, &VideoSaveOptionsPage::updateFilenamePreview);

    m_ui->captureInstructionLabel->setText(CaptureInstructions::text(false));
    connect(m_ui->captureInstructionLabel, &QLabel::linkActivated, this, [this](const QString &link) {
        if (link == u"showmore"_s) {
            m_ui->captureInstructionLabel->setText(CaptureInstructions::text(true));
        } else if (link == u"showfewer"_s) {
            m_ui->captureInstructionLabel->setText(CaptureInstructions::text(false));
        } else {
            m_ui->kcfg_videoFilenameTemplate->insert(link);
        }
    });
}

VideoSaveOptionsPage::~VideoSaveOptionsPage() = default;

void VideoSaveOptionsPage::updateFilenamePreview()
{
    const auto extension = m_videoFormatComboBox->currentData(VideoFormatModel::ExtensionRole).toString();
    const auto templateBasename = m_ui->kcfg_videoFilenameTemplate->text();
    auto previewText = Filename::previewText(templateBasename + u'.' + extension);
    const bool warn = Filename::showWarning(previewText);
    if (warn) {
        previewText = Filename::highlightedPreviewText(previewText);
    }
    m_ui->preview->setText(previewText);
    Filename::warningToolTip(m_ui->preview, warn);
}

#include "moc_VideoSaveOptionsPage.cpp"
