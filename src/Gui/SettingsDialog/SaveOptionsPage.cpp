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

#include <QLineEdit>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStandardPaths>
#include <QComboBox>
#include <QImageWriter>

#include <KLocalizedString>
#include <KIOWidgets/KUrlRequester>

#include "SpectacleConfig.h"

SaveOptionsPage::SaveOptionsPage(QWidget *parent) :
    SettingsPage(parent)
{
    // set up the layout. start with the directory

    QGroupBox *dirGroup = new QGroupBox(i18n("Default Save Location"), this);
    QVBoxLayout *dirLayout = new QVBoxLayout;
    dirGroup->setLayout(dirLayout);

    QHBoxLayout *urlRequesterLayout = new QHBoxLayout;
    urlRequesterLayout->addWidget(new QLabel(i18n("Location:"), this));

    mUrlRequester = new KUrlRequester;
    mUrlRequester->setMode(KFile::Directory);
    connect(mUrlRequester, &KUrlRequester::textChanged, this, &SaveOptionsPage::markDirty);
    urlRequesterLayout->addWidget(mUrlRequester);

    dirLayout->addLayout(urlRequesterLayout);

    // filename chooser text field

    QGroupBox *fmtGroup = new QGroupBox(i18n("Default Save Filename"));
    QVBoxLayout *fmtLayout = new QVBoxLayout;
    fmtGroup->setLayout(fmtLayout);

    QHBoxLayout *saveNameLayout = new QHBoxLayout;
    saveNameLayout->addWidget(new QLabel(i18n("Filename:"), this));

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
    saveNameLayout->addWidget(mSaveNameFormat);

    mSaveImageFormat = new QComboBox;
    mSaveImageFormat->addItems([&](){
        QStringList items;
        Q_FOREACH(auto fmt, QImageWriter::supportedImageFormats()) {
            items.append(QString::fromLocal8Bit(fmt).toUpper());
        }
        return items;
    }());
    connect(mSaveImageFormat, &QComboBox::currentTextChanged, this, &SaveOptionsPage::markDirty);
    saveNameLayout->addWidget(mSaveImageFormat);

    fmtLayout->addLayout(saveNameLayout);

    // now the save filename format layout

    const QString helpText = i18n(
        "<p>You can use the following placeholders in the filename, which will be replaced "
        "with actual text when the file is saved:</p>"

        "<blockquote>"
            "<b>%Y</b>: Year (4 digit)<br />"
            "<b>%y</b>: Year (2 digit)<br />"
            "<b>%M</b>: Month<br />"
            "<b>%D</b>: Day<br />"
            "<b>%H</b>: Hour<br />"
            "<b>%m</b>: Minute<br />"
            "<b>%S</b>: Second"
        "</blockquote>"
    );

    QLabel *fmtHelpText = new QLabel(helpText, this);
    fmtHelpText->setWordWrap(true);
    fmtHelpText->setTextFormat(Qt::RichText);
    fmtLayout->addWidget(fmtHelpText);

    // read in the data

    resetChanges();

    // finish up with the main layout

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mainLayout->addWidget(dirGroup);
    mainLayout->addWidget(fmtGroup);

    mainLayout->addStretch(4);
    setLayout(mainLayout);
}

void SaveOptionsPage::markDirty(const QString &text)
{
    Q_UNUSED(text);
    mChangesMade = true;
}

void SaveOptionsPage::saveChanges()
{
    // bring up the configuration reader

    SpectacleConfig *cfgManager = SpectacleConfig::instance();

    // save the data

    cfgManager->setAutoSaveLocation(mUrlRequester->url().toDisplayString(QUrl::PreferLocalFile));
    cfgManager->setAutoSaveFilenameFormat(mSaveNameFormat->text());
    cfgManager->setSaveImageFormat(mSaveImageFormat->currentText().toLower());

    // done

    mChangesMade = false;
}

void SaveOptionsPage::resetChanges()
{
    // bring up the configuration reader

    SpectacleConfig *cfgManager = SpectacleConfig::instance();

    // read in the data

    mSaveNameFormat->setText(cfgManager->autoSaveFilenameFormat());
    mUrlRequester->setUrl(QUrl::fromUserInput(cfgManager->autoSaveLocation()));

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
