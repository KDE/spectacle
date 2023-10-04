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

ImageSaveOptionsPage::ImageSaveOptionsPage(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui_ImageSaveOptions)
{
    m_ui->setupUi(this);

    connect(m_ui->kcfg_imageFilenameFormat, &QLineEdit::textEdited, this, [&](const QString &newText) {
        QString fmt;
        const auto imageFormats = QImageWriter::supportedImageFormats();
        for (const auto &item : imageFormats) {
            fmt = QString::fromLocal8Bit(item);
            if (newText.endsWith(QLatin1Char('.') + fmt, Qt::CaseInsensitive)) {
                QString txtCopy = newText;
                txtCopy.chop(fmt.length() + 1);
                m_ui->kcfg_imageFilenameFormat->setText(txtCopy);
                m_ui->kcfg_preferredImageFormat->setCurrentIndex(m_ui->kcfg_preferredImageFormat->findText(fmt.toUpper()));
            }
        }
    });
    connect(m_ui->kcfg_imageFilenameFormat, &QLineEdit::textChanged, this, &ImageSaveOptionsPage::updateFilenamePreview);

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

    QString captureInstruction = i18n(
        "You can use the following placeholders in the filename, which will be replaced "
        "with actual text when the file is saved:<blockquote>");
    for (auto option = ExportManager::filenamePlaceholders.cbegin(); option != ExportManager::filenamePlaceholders.cend(); ++option) {
        captureInstruction += QStringLiteral("<a href=%1>%1</a>: %2<br>").arg(option.key(), option.value().toString());
    }
    captureInstruction += QLatin1String("<a href='/'>/</a>: ") + i18n("To save to a sub-folder");
    captureInstruction += QStringLiteral("</blockquote>");
    m_ui->captureInstructionLabel->setText(captureInstruction);
    connect(m_ui->captureInstructionLabel, &QLabel::linkActivated, this, [this](const QString &placeholder) {
        m_ui->kcfg_imageFilenameFormat->insert(placeholder);
    });

    m_ui->imageCompressionQualityHelpLable->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
}

ImageSaveOptionsPage::~ImageSaveOptionsPage() = default;

void ImageSaveOptionsPage::updateFilenamePreview()
{
    const auto extension = m_ui->kcfg_preferredImageFormat->currentText().toLower();
    const auto templateBasename = m_ui->kcfg_imageFilenameFormat->text();
    ::updateFilenamePreview(m_ui->preview, templateBasename + u'.' + extension);
}

#include "moc_ImageSaveOptionsPage.cpp"
