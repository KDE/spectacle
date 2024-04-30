/*
 *  SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ImageSaveOptionsPage.h"

#include "ExportManager.h"
#include "SaveOptionsUtils.h"
#include "ui_ImageSaveOptions.h"

#include <KLocalizedString>

#include <QCheckBox>
#include <QComboBox>
#include <QFontDatabase>
#include <QImageWriter>
#include <QLabel>
#include <QLineEdit>

using namespace Qt::StringLiterals;

ImageSaveOptionsPage::ImageSaveOptionsPage(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui_ImageSaveOptions)
{
    m_ui->setupUi(this);

    m_ui->imageCompressionQualityHelpLable->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    const int sliderSpinboxHeightDiff = m_ui->qualitySpinner->sizeHint().height() - m_ui->kcfg_imageCompressionQuality->sizeHint().height();
    const int smallLabelLineEditHeightDiff =
        m_ui->kcfg_imageFilenameTemplate->sizeHint().height() - m_ui->imageCompressionQualityHelpLable->sizeHint().height();
    m_ui->qualityVLayout->setContentsMargins({
        0,
        std::max(0, qRound(sliderSpinboxHeightDiff / 2.0)),
        0,
        std::max(0, qRound(smallLabelLineEditHeightDiff / 2.0)),
    });

    connect(m_ui->kcfg_imageFilenameTemplate, &QLineEdit::textEdited, this, [&](const QString &newText) {
        QString fmt;
        const auto imageFormats = QImageWriter::supportedImageFormats();
        for (const auto &item : imageFormats) {
            fmt = QString::fromLocal8Bit(item);
            if (newText.endsWith(u'.' + fmt, Qt::CaseInsensitive)) {
                QString txtCopy = newText;
                txtCopy.chop(fmt.length() + 1);
                m_ui->kcfg_imageFilenameTemplate->setText(txtCopy);
                m_ui->kcfg_preferredImageFormat->setCurrentIndex(m_ui->kcfg_preferredImageFormat->findText(fmt.toUpper()));
            }
        }
    });
    connect(m_ui->kcfg_imageFilenameTemplate, &QLineEdit::textChanged, this, &ImageSaveOptionsPage::updateFilenamePreview);

    m_ui->preview->setFixedHeight(m_ui->kcfg_imageFilenameTemplate->height());

    m_ui->kcfg_preferredImageFormat->addItems([&]() {
        QStringList items;
        const auto formats = QImageWriter::supportedImageFormats();
        items.reserve(formats.count());
        for (const auto &fmt : formats) {
            items.append(QString::fromLocal8Bit(fmt).toUpper());
        }
        return items;
    }());
    connect(m_ui->kcfg_preferredImageFormat, &QComboBox::currentTextChanged, this, &ImageSaveOptionsPage::updateFilenamePreview);

    m_ui->captureInstructionLabel->setText(CaptureInstructions::text(false));
    connect(m_ui->captureInstructionLabel, &QLabel::linkActivated, this, [this](const QString &link) {
        if (link == u"showmore"_s) {
            m_ui->captureInstructionLabel->setText(CaptureInstructions::text(true));
        } else if (link == u"showfewer"_s) {
            m_ui->captureInstructionLabel->setText(CaptureInstructions::text(false));
        } else {
            m_ui->kcfg_imageFilenameTemplate->insert(link);
        }
    });
}

ImageSaveOptionsPage::~ImageSaveOptionsPage() = default;

void ImageSaveOptionsPage::updateFilenamePreview()
{
    const auto extension = m_ui->kcfg_preferredImageFormat->currentText().toLower();
    const auto templateBasename = m_ui->kcfg_imageFilenameTemplate->text();
    auto previewText = Filename::previewText(templateBasename + u'.' + extension);
    const bool warn = Filename::showWarning(previewText);
    if (warn) {
        previewText = Filename::highlightedPreviewText(previewText);
    }
    m_ui->preview->setText(previewText);
    Filename::warningToolTip(m_ui->preview, warn);
}

#include "moc_ImageSaveOptionsPage.cpp"
