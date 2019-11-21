/*
 *  Copyright 2019 David Redondo <kde@david-redondo.de>
 *  Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "SaveOptionsPage.h"

#include "SpectacleCommon.h"
#include "ExportManager.h"

#include <KIOWidgets/KUrlRequester>
#include <KLocalizedString>

#include <QLineEdit>
#include <QLabel>
#include <QSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QImageWriter>
#include <QCheckBox>

SaveOptionsPage::SaveOptionsPage(QWidget *parent) : QWidget(parent)
{
    QFormLayout *mainLayout = new QFormLayout;
    setLayout(mainLayout);

    // Save location
    auto urlRequester = new KUrlRequester(this);
    urlRequester->setObjectName(QStringLiteral("kcfg_defaultSaveLocation"));
    urlRequester->setMode(KFile::Directory);
    mainLayout->addRow(i18n("Save Location:"), urlRequester);

    // copy file location to clipboard after saving
    auto copyPathToClipboard = new QCheckBox(i18n("Copy file location to clipboard after saving"), this);
    copyPathToClipboard->setObjectName(QStringLiteral("kcfg_copySaveLocation"));
    mainLayout->addRow(QString(), copyPathToClipboard);

    mainLayout->addItem(new QSpacerItem(0, 18, QSizePolicy::Fixed, QSizePolicy::Fixed));

    // Compression quality slider and current value display
    QHBoxLayout *sliderHorizLayout = new QHBoxLayout(this);
    QVBoxLayout *sliderVertLayout = new QVBoxLayout(this);

    // Current value
    auto qualitySpinner = new QSpinBox(this);
    qualitySpinner->setSuffix(QString::fromUtf8("%"));
    qualitySpinner->setRange(0, 100);
    qualitySpinner->setObjectName(QStringLiteral("kcfg_compressionQuality"));

    // Slider
    auto qualitySlider = new QSlider(Qt::Horizontal, this);
    qualitySlider->setRange(0, 100);
    qualitySlider->setSliderPosition(qualitySpinner->value());
    qualitySlider->setTracking(true);
    connect(qualitySlider, &QSlider::valueChanged, this, [=](int value) {
        qualitySpinner->setValue(value);
    });
    connect(qualitySpinner, QOverload<int>::of(&QSpinBox::valueChanged), this, [=] (int value) {qualitySlider->setValue(value);});
    sliderHorizLayout->addWidget(qualitySlider);
    sliderHorizLayout->addWidget(qualitySpinner);

    sliderVertLayout->addLayout(sliderHorizLayout);

    QLabel *qualitySliderDescription = new QLabel(this);
    qualitySliderDescription->setText(i18n("Choose the image quality when saving with lossy image formats like JPEG"));

    sliderVertLayout->addWidget(qualitySliderDescription);

    mainLayout->addRow(i18n("Compression Quality:"), sliderVertLayout);

    mainLayout->addItem(new QSpacerItem(0, 18, QSizePolicy::Fixed, QSizePolicy::Fixed));

    // filename chooser text field
    QHBoxLayout *saveFieldLayout = new QHBoxLayout(this);
    mSaveNameFormat = new QLineEdit(this);
    mSaveNameFormat->setObjectName(QStringLiteral("kcfg_saveFilenameFormat"));

    connect(mSaveNameFormat, &QLineEdit::textEdited, this, [&](const QString &newText) {
        QString fmt;
        const auto imageFormats = QImageWriter::supportedImageFormats();
        for (const auto &item : imageFormats) {
            fmt = QString::fromLocal8Bit(item);
            if (newText.endsWith(QLatin1Char('.') + fmt, Qt::CaseInsensitive)) {
                QString txtCopy = newText;
                txtCopy.chop(fmt.length() + 1);
                mSaveNameFormat->setText(txtCopy);
                mSaveImageFormat->setCurrentIndex(mSaveImageFormat->findText(fmt.toUpper()));
            }
        }
    });
    connect(mSaveNameFormat, &QLineEdit::textChanged,this, &SaveOptionsPage::updateFilenamePreview);
    mSaveNameFormat->setPlaceholderText(QStringLiteral("%d"));
    saveFieldLayout->addWidget(mSaveNameFormat);

    mSaveImageFormat = new QComboBox(this);
    mSaveImageFormat->setObjectName(QStringLiteral("kcfg_defaultSaveImageFormat"));
    mSaveImageFormat->setProperty("kcfg_property", QByteArray("currentText"));
    mSaveImageFormat->addItems([&](){
        QStringList items;
        const auto formats = QImageWriter::supportedImageFormats();
        for (const auto &fmt : formats) {
            items.append(QString::fromLocal8Bit(fmt).toUpper());
        }
        return items;
    }());
    connect(mSaveImageFormat, &QComboBox::currentTextChanged, this, &SaveOptionsPage::updateFilenamePreview);
    saveFieldLayout->addWidget(mSaveImageFormat);
    mainLayout->addRow(i18n("Filename:"), saveFieldLayout);

    mPreviewLabel = new QLabel(this);
    mainLayout->addRow(i18nc("Preview of the user configured filename", "Preview:"), mPreviewLabel);
    // now the save filename format layout
    QString helpText = i18n(
        "You can use the following placeholders in the filename, which will be replaced "
        "with actual text when the file is saved:<blockquote>"
    );
    for (auto option = ExportManager::filenamePlaceholders.cbegin();
         option != ExportManager::filenamePlaceholders.cend(); ++option) {
        helpText += QStringLiteral("<a href=%1>%1</a>: %2<br>").arg(option.key(),
                                                                    option.value().toString());
    }
    helpText += QLatin1String("<a href='/'>/</a>: ") + i18n("To save to a sub-folder");
    helpText += QStringLiteral("</blockquote>");
    QLabel *fmtHelpText = new QLabel(helpText, this);
    fmtHelpText->setWordWrap(true);
    fmtHelpText->setTextFormat(Qt::RichText);
    fmtHelpText->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    connect(fmtHelpText, &QLabel::linkActivated, this, [this](const QString& placeholder) {
        mSaveNameFormat->insert(placeholder);
    });
    mainLayout->addWidget(fmtHelpText);
}

void SaveOptionsPage::updateFilenamePreview()
{
    auto lExportManager = ExportManager::instance();
    lExportManager->setWindowTitle(QStringLiteral("Spectacle"));
    Spectacle::CaptureMode lOldMode = lExportManager->captureMode();

    // If the grabMode is not one of those below we need to change it to have the placeholder
    // replaced by the window title
    bool lSwitchGrabMode = !(lOldMode == Spectacle::CaptureMode::ActiveWindow ||
                                      lOldMode == Spectacle::CaptureMode::TransientWithParent ||
                                      lOldMode == Spectacle::CaptureMode::WindowUnderCursor);
    if (lSwitchGrabMode) {
       lExportManager->setCaptureMode(Spectacle::CaptureMode::ActiveWindow);
    }
    const QString lFileName = lExportManager->formatFilename(mSaveNameFormat->text());
    mPreviewLabel->setText(xi18nc("@info", "<filename>%1.%2</filename>", lFileName, mSaveImageFormat->currentText().toLower()));
    if (lSwitchGrabMode) {
        lExportManager->setCaptureMode(lOldMode);
    }
}
