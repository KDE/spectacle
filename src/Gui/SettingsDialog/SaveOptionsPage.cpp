/*
 *  SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SaveOptionsPage.h"

#include "ExportManager.h"
#include "SpectacleCommon.h"
#include "ui_SaveOptions.h"

#include <KIOWidgets/KUrlRequester>
#include <KLocalizedString>

#include <QCheckBox>
#include <QComboBox>
#include <QFontDatabase>
#include <QImageWriter>
#include <QLabel>
#include <QLineEdit>

SaveOptionsPage::SaveOptionsPage(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui_SaveOptions)
{
    m_ui->setupUi(this);

    connect(m_ui->kcfg_saveFilenameFormat, &QLineEdit::textEdited, this, [&](const QString &newText) {
        QString fmt;
        const auto imageFormats = QImageWriter::supportedImageFormats();
        for (const auto &item : imageFormats) {
            fmt = QString::fromLocal8Bit(item);
            if (newText.endsWith(QLatin1Char('.') + fmt, Qt::CaseInsensitive)) {
                QString txtCopy = newText;
                txtCopy.chop(fmt.length() + 1);
                m_ui->kcfg_saveFilenameFormat->setText(txtCopy);
                m_ui->kcfg_defaultSaveImageFormat->setCurrentIndex(m_ui->kcfg_defaultSaveImageFormat->findText(fmt.toUpper()));
            }
        }
    });
    connect(m_ui->kcfg_saveFilenameFormat, &QLineEdit::textChanged, this, &SaveOptionsPage::updateFilenamePreview);

    m_ui->kcfg_defaultSaveImageFormat->addItems([&]() {
        QStringList items;
        const auto formats = QImageWriter::supportedImageFormats();
        items.reserve(formats.count());
        for (const auto &fmt : formats) {
            items.append(QString::fromLocal8Bit(fmt).toUpper());
        }
        return items;
    }());
    connect(m_ui->kcfg_defaultSaveImageFormat, &QComboBox::currentTextChanged, this, &SaveOptionsPage::updateFilenamePreview);

    QString helpText = i18n(
        "You can use the following placeholders in the filename, which will be replaced "
        "with actual text when the file is saved:<blockquote>");
    for (auto option = ExportManager::filenamePlaceholders.cbegin(); option != ExportManager::filenamePlaceholders.cend(); ++option) {
        helpText += QStringLiteral("<a href=%1>%1</a>: %2<br>").arg(option.key(), option.value().toString());
    }
    helpText += QLatin1String("<a href='/'>/</a>: ") + i18n("To save to a sub-folder");
    helpText += QStringLiteral("</blockquote>");
    m_ui->helpTextLabel->setText(helpText);
    connect(m_ui->helpTextLabel, &QLabel::linkActivated, this, [this](const QString &placeholder) {
        m_ui->kcfg_saveFilenameFormat->insert(placeholder);
    });

    m_ui->compressionQualityHelpLable->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
}

SaveOptionsPage::~SaveOptionsPage() = default;

void SaveOptionsPage::updateFilenamePreview()
{
    auto lExportManager = ExportManager::instance();
    lExportManager->setWindowTitle(QStringLiteral("Spectacle"));
    Spectacle::CaptureMode lOldMode = lExportManager->captureMode();

    // If the grabMode is not one of those below we need to change it to have the placeholder
    // replaced by the window title
    bool lSwitchGrabMode = !(lOldMode == Spectacle::CaptureMode::ActiveWindow || lOldMode == Spectacle::CaptureMode::TransientWithParent
                             || lOldMode == Spectacle::CaptureMode::WindowUnderCursor);
    if (lSwitchGrabMode) {
        lExportManager->setCaptureMode(Spectacle::CaptureMode::ActiveWindow);
    }
    const QString lFileName = lExportManager->formatFilename(m_ui->kcfg_saveFilenameFormat->text());
    m_ui->preview->setText(xi18nc("@info", "<filename>%1.%2</filename>", lFileName, m_ui->kcfg_defaultSaveImageFormat->currentText().toLower()));
    if (lSwitchGrabMode) {
        lExportManager->setCaptureMode(lOldMode);
    }
}
