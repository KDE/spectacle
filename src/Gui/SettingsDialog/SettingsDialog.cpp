/*
 *  SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SettingsDialog.h"

#include "GeneralOptionsPage.h"
#include "SaveOptionsPage.h"
#include "ShortcutsOptionsPage.h"
#include "settings.h"

#include <QFontDatabase>

#include <KLocalizedString>
#include <KShortcutWidget>
#include <qevent.h>

SettingsDialog::SettingsDialog(QWidget *parent)
    : KConfigDialog(parent, QStringLiteral("settings"), Settings::self())
    , mShortcutsPage(new ShortcutsOptionsPage(this))
{
    addPage(new GeneralOptionsPage(this), Settings::self(), i18n("General"), QStringLiteral("spectacle"));
    addPage(new SaveOptionsPage(this), Settings::self(), i18n("Save"), QStringLiteral("document-save"));
    addPage(mShortcutsPage, i18n("Shortcuts"), QStringLiteral("preferences-desktop-keyboard"));
    connect(mShortcutsPage, &ShortcutsOptionsPage::shortCutsChanged, this, [this] {
        updateButtons();
    });
    connect(this, &KConfigDialog::currentPageChanged, this, &SettingsDialog::updateButtons);
}

QSize SettingsDialog::sizeHint() const
{
    // Take the font size into account for the window size, as we do for UI elements
    const float fontSize = QFontDatabase::systemFont(QFontDatabase::GeneralFont).pointSizeF();
    return QSize(qRound(58 * fontSize), qRound(62 * fontSize));
}

bool SettingsDialog::hasChanged()
{
    return mShortcutsPage->isModified() || KConfigDialog::hasChanged();
}

bool SettingsDialog::isDefault()
{
    return currentPage()->name() != i18n("Shortcuts") && KConfigDialog::isDefault();
}

void SettingsDialog::updateSettings()
{
    KConfigDialog::updateSettings();
    mShortcutsPage->saveChanges();
}

void SettingsDialog::updateWidgets()
{
    KConfigDialog::updateWidgets();
    mShortcutsPage->resetChanges();
}

void SettingsDialog::updateWidgetsDefault()
{
    KConfigDialog::updateWidgetsDefault();
    mShortcutsPage->defaults();
}

void SettingsDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        return;
    }
    QDialog::keyPressEvent(event);
}

void SettingsDialog::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        QDialog::reject();
        return;
    }
    QDialog::keyReleaseEvent(event);
}
