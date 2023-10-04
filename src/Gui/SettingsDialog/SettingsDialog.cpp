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
#include <QScreen>
#include <QStyle>
#include <QWindow>

#include <KLocalizedString>
#include <KShortcutWidget>

using namespace Qt::StringLiterals;

SettingsDialog::SettingsDialog(QWidget *parent)
    : KConfigDialog(parent, "settings"_L1, Settings::self())
    , m_generalPage(new GeneralOptionsPage(this))
    , m_savePage(new SaveOptionsPage(this))
    , m_shortcutsPage(new ShortcutsOptionsPage(this))
{
    setFaceType(KPageDialog::List);
    addPage(m_generalPage, Settings::self(), i18nc("Settings category", "General"), "spectacle"_L1);
    addPage(m_savePage, Settings::self(), i18nc("Settings category", "Save"), "document-save"_L1);
    addPage(m_shortcutsPage, i18nc("Settings category", "Shortcuts"), "preferences-desktop-keyboard"_L1);
    connect(m_shortcutsPage, &ShortcutsOptionsPage::shortCutsChanged, this, [this] {
        updateButtons();
    });
    connect(this, &KConfigDialog::currentPageChanged, this, &SettingsDialog::updateButtons);
}

QSize SettingsDialog::sizeHint() const
{
    // Avoid having pages that need to be scrolled,
    // unless size is larger than available screen height.
    const auto headerSize = pageWidget()->pageHeader()->sizeHint();
    const auto footerSize = pageWidget()->pageFooter()->sizeHint();
    auto sh = m_generalPage->sizeHint();
    sh = sh.expandedTo(m_savePage->sizeHint());
    sh = sh.expandedTo(m_shortcutsPage->sizeHint());
    sh.rheight() += headerSize.height() + footerSize.height()
                 + style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing) * 2;
    sh = KConfigDialog::sizeHint().expandedTo(sh);
    const auto screenHeight = screen() ? screen()->availableGeometry().height() : 0;
    sh.setHeight(std::min(sh.height(), screenHeight));
    return sh;
}

void SettingsDialog::showEvent(QShowEvent *event)
{
    auto parent = parentWidget();
    bool onTop = parent && parent->windowHandle()->flags().testFlag(Qt::WindowStaysOnTopHint);
    windowHandle()->setFlag(Qt::WindowStaysOnTopHint, onTop);
    KConfigDialog::showEvent(event);
}

bool SettingsDialog::hasChanged()
{
    return m_shortcutsPage->isModified() || KConfigDialog::hasChanged();
}

bool SettingsDialog::isDefault()
{
    return currentPage()->name() != i18n("Shortcuts") && KConfigDialog::isDefault();
}

void SettingsDialog::updateSettings()
{
    KConfigDialog::updateSettings();
    m_shortcutsPage->saveChanges();
}

void SettingsDialog::updateWidgets()
{
    KConfigDialog::updateWidgets();
    m_shortcutsPage->resetChanges();
}

void SettingsDialog::updateWidgetsDefault()
{
    KConfigDialog::updateWidgetsDefault();
    m_shortcutsPage->defaults();
}

#include "moc_SettingsDialog.cpp"
