/*
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
#include "SpectacleConfig.h"
#include "ExportManager.h"

#include <KIOWidgets/KUrlRequester>
#include <KLocalizedString>

#include <QLineEdit>
#include <QLabel>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QComboBox>
#include <QImageWriter>
#include <QCheckBox>

SaveOptionsPage::SaveOptionsPage(QWidget *parent) :
    SettingsPage(parent)
{
    QFormLayout *mainLayout = new QFormLayout;
    setLayout(mainLayout);

    // Save location
    mUrlRequester = new KUrlRequester;
    mUrlRequester->setMode(KFile::Directory);
    connect(mUrlRequester, &KUrlRequester::textChanged, this, &SaveOptionsPage::markDirty);
    mainLayout->addRow(i18n("Save Location:"), mUrlRequester);

    // copy file location to clipboard after saving
    mCopyPathToClipboard = new QCheckBox(i18n("Copy file location to clipboard after saving"), this);
    connect(mCopyPathToClipboard, &QCheckBox::toggled, this, &SaveOptionsPage::markDirty);
    mainLayout->addRow(QString(), mCopyPathToClipboard);


    mainLayout->addItem(new QSpacerItem(0, 18, QSizePolicy::Fixed, QSizePolicy::Fixed));

    // Compression quality slider and current value display
    QHBoxLayout *sliderHorizLayout = new QHBoxLayout();
    QVBoxLayout *sliderVertLayout = new QVBoxLayout();

    // Current value
    QLabel *qualityValue = new QLabel();
    qualityValue->setNum(SpectacleConfig::instance()->compressionQuality());
    qualityValue->setMinimumWidth(qualityValue->fontInfo().pointSize()*3);

    // Slider
    mQualitySlider = new QSlider(Qt::Horizontal);
    mQualitySlider->setRange(0, 100);
    mQualitySlider->setTickInterval(5);
    mQualitySlider->setSliderPosition(SpectacleConfig::instance()->compressionQuality());
    mQualitySlider->setTickPosition(QSlider::TicksBelow);
    mQualitySlider->setTracking(true);
    connect(mQualitySlider, &QSlider::valueChanged, [=](int value) {
        qualityValue->setNum(value);
        markDirty();
    });

    sliderHorizLayout->addWidget(mQualitySlider);
    sliderHorizLayout->addWidget(qualityValue);

    sliderVertLayout->addLayout(sliderHorizLayout);

    QLabel *qualitySliderDescription = new QLabel();
    qualitySliderDescription->setText(i18n("Choose the image quality when saving with lossy image formats like JPEG"));

    sliderVertLayout->addWidget(qualitySliderDescription);

    mainLayout->addRow(i18n("Compression Quality:"), sliderVertLayout);

    mainLayout->addItem(new QSpacerItem(0, 18, QSizePolicy::Fixed, QSizePolicy::Fixed));

    // filename chooser text field
    QHBoxLayout *saveFieldLayout = new QHBoxLayout;
    mSaveNameFormat = new QLineEdit;
    connect(mSaveNameFormat, &QLineEdit::textEdited, this, &SaveOptionsPage::markDirty);
    connect(mSaveNameFormat, &QLineEdit::textEdited, [&](const QString &newText) {
        QString fmt;
        Q_FOREACH(auto item, QImageWriter::supportedImageFormats()) {
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

    mSaveImageFormat = new QComboBox;
    mSaveImageFormat->addItems([&](){
        QStringList items;
        Q_FOREACH(auto fmt, QImageWriter::supportedImageFormats()) {
            items.append(QString::fromLocal8Bit(fmt).toUpper());
        }
        return items;
    }());
    connect(mSaveImageFormat, &QComboBox::currentTextChanged, this, &SaveOptionsPage::markDirty);
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
    helpText += QStringLiteral("<a href='/'>/</a>: ") + i18n("To save to a sub-folder");
    helpText += QStringLiteral("</blockquote>");
    QLabel *fmtHelpText = new QLabel(helpText, this);
    fmtHelpText->setWordWrap(true);
    fmtHelpText->setTextFormat(Qt::RichText);
    fmtHelpText->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    connect(fmtHelpText, &QLabel::linkActivated, [this](const QString& placeholder) {
        mSaveNameFormat->insert(placeholder);
    });
    mainLayout->addWidget(fmtHelpText);

    // read in the data
    resetChanges();
}

void SaveOptionsPage::markDirty()
{
    mChangesMade = true;
}

void SaveOptionsPage::saveChanges()
{
    // bring up the configuration reader

    SpectacleConfig *cfgManager = SpectacleConfig::instance();

    // save the data

    cfgManager->setDefaultSaveLocation(mUrlRequester->url());
    cfgManager->setAutoSaveFilenameFormat(mSaveNameFormat->text());
    cfgManager->setSaveImageFormat(mSaveImageFormat->currentText().toLower());
    cfgManager->setCopySaveLocationToClipboard(mCopyPathToClipboard->checkState() == Qt::Checked);
    cfgManager->setCompressionQuality(mQualitySlider->value());

    // done

    mChangesMade = false;
}

void SaveOptionsPage::resetChanges()
{
    // bring up the configuration reader

    SpectacleConfig *cfgManager = SpectacleConfig::instance();

    // read in the data

    mSaveNameFormat->setText(cfgManager->autoSaveFilenameFormat());
    mUrlRequester->setUrl(cfgManager->defaultSaveLocation());
    mCopyPathToClipboard->setChecked(cfgManager->copySaveLocationToClipboard());
    mQualitySlider->setSliderPosition(cfgManager->compressionQuality());

    // read in the save image format and calculate its index

    {
        int index = mSaveImageFormat->findText(cfgManager->saveImageFormat().toUpper());
        if (index >= 0) {
            mSaveImageFormat->setCurrentIndex(index);
        }
    }

    // done

    mChangesMade = false;
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
