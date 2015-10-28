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

#include "KSSaveConfigDialog.h"

KSSaveConfigDialog::KSSaveConfigDialog(QWidget *parent) :
    QDialog(parent)
{
    // set the window properties first

    setWindowTitle(i18n("Configure Save Options"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(500, 600);

    // bring up the configuration reader

    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("spectaclerc"));
    KConfigGroup generalConfig = KConfigGroup(config, "General");

    // set up the layout. start with the directory

    QGroupBox *dirGroup = new QGroupBox(i18n("Default Save Directory"));
    QVBoxLayout *dirLayout = new QVBoxLayout;
    dirGroup->setLayout(dirLayout);
    dirGroup->setStyleSheet(QStringLiteral("QGroupBox { font-weight: bold; }"));

    QLabel *dirHelpText = new QLabel;
    dirHelpText->setWordWrap(true);
    dirHelpText->setText(i18n("Set the directory where you'd like to save your screenshots when you press "
                              "<b>Save</b> or <b>Save & Exit</b>."));
    dirLayout->addWidget(dirHelpText);

    QHBoxLayout *urlRequesterLayout = new QHBoxLayout;
    urlRequesterLayout->addWidget(new QLabel(i18n("Location:")));

    mUrlRequester = new KUrlRequester;
    mUrlRequester->setMode(KFile::Directory);
    const QString path = generalConfig.readPathEntry("default-save-location",
                                                     QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    mUrlRequester->setUrl(QUrl::fromUserInput(path));
    urlRequesterLayout->addWidget(mUrlRequester);

    dirLayout->addLayout(urlRequesterLayout);

    // now the save filename format layout

    QGroupBox *fmtGroup = new QGroupBox(i18n("Default Save Filename"));
    QVBoxLayout *fmtLayout = new QVBoxLayout;
    fmtGroup->setLayout(fmtLayout);
    fmtGroup->setStyleSheet(QStringLiteral("QGroupBox { font-weight: bold; }"));

    const QString helpText = i18n(
        "<p>Set a default filename for saved screenshots.</p>"

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

        "<p>You don't need to enter a filetype extension. By default, screenshots are always saved "
        "as a <b>PNG (Portable Network Graphics)</b> image with a <b>.png</b> extension.</p>"

        "<p>If a file with this name already exists, a serial number will be appended to the filename. "
        "For example, if the filename is \"Screenshot\", and \"Screenshot.png\" already "
        "exists, the image will be saved as \"Screenshot-1.png\".</p>"
    );

    QLabel *fmtHelpText = new QLabel;
    fmtHelpText->setWordWrap(true);
    fmtHelpText->setText(helpText);
    fmtHelpText->setTextFormat(Qt::RichText);
    fmtLayout->addWidget(fmtHelpText);

    QHBoxLayout *saveNameLayout = new QHBoxLayout;
    saveNameLayout->addWidget(new QLabel(i18n("Filename:")));

    mSaveNameFormat = new QLineEdit;
    const QString saveFmt = generalConfig.readEntry("save-filename-format", "Screenshot_%Y%M%D_%H%m%S");
    mSaveNameFormat->setText(saveFmt);
    saveNameLayout->addWidget(mSaveNameFormat);

    fmtLayout->addLayout(saveNameLayout);

    // finish up with the main layout

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mainLayout->addWidget(dirGroup);
    mainLayout->addWidget(fmtGroup);

    mDialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(mDialogButtonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &KSSaveConfigDialog::accept);
    connect(mDialogButtonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &KSSaveConfigDialog::reject);
    mainLayout->addWidget(mDialogButtonBox);

    setLayout(mainLayout);
}

KSSaveConfigDialog::~KSSaveConfigDialog()
{}

void KSSaveConfigDialog::accept()
{
    // bring up the configuration reader

    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("spectaclerc"));
    KConfigGroup generalConfig = KConfigGroup(config, "General");

    // save the data

    generalConfig.writePathEntry("default-save-location", mUrlRequester->url().toDisplayString(QUrl::PreferLocalFile));
    generalConfig.writeEntry("save-filename-format", mSaveNameFormat->text());

    // done

    emit done(QDialog::Accepted);
}
