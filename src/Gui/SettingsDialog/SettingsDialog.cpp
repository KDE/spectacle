/*
 *  SPDX-FileCopyrightText: 2025 Jhair Paris <dev@jhairparis.com>
 *  SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SettingsDialog.h"

#include "GeneralOptionsPage.h"
#include "ImageSaveOptionsPage.h"
#include "OcrLanguageSelector.h"
#include "ShortcutsOptionsPage.h"
#include "VideoSaveOptionsPage.h"
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
    , m_imagesPage(new ImageSaveOptionsPage(this))
    , m_videosPage(new VideoSaveOptionsPage(this))
    , m_shortcutsPage(new ShortcutsOptionsPage(this))
{
    setFaceType(KPageDialog::List);
    addPage(m_generalPage, Settings::self(), i18nc("Settings category", "General"), "spectacle"_L1);
    addPage(m_imagesPage, Settings::self(), i18nc("Settings category", "Image Saving"), "image-x-generic"_L1);
    addPage(m_videosPage, Settings::self(), i18nc("Settings category", "Video Saving"), "video-x-generic"_L1);
    addPage(m_shortcutsPage, i18nc("Settings category", "Shortcuts"), "preferences-desktop-keyboard"_L1);
    connect(m_shortcutsPage, &ShortcutsOptionsPage::shortCutsChanged, this, [this] {
        updateButtons();
    });
    connect(m_generalPage, &GeneralOptionsPage::ocrLanguageChanged, this, [this] {
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
    sh = sh.expandedTo(m_imagesPage->sizeHint());
    sh = sh.expandedTo(m_videosPage->sizeHint());
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
    
    m_generalPage->refreshOcrLanguageSettings();
    
    KConfigDialog::showEvent(event);
}

bool SettingsDialog::hasChanged()
{
    return m_shortcutsPage->isModified() || m_generalPage->ocrLanguageSelector()->hasChanges() || KConfigDialog::hasChanged();
}

bool SettingsDialog::isDefault()
{
    return currentPage()->name() != i18n("Shortcuts") && m_generalPage->ocrLanguageSelector()->isDefault() && KConfigDialog::isDefault();
}

void SettingsDialog::updateSettings()
{
    KConfigDialog::updateSettings();
    m_shortcutsPage->saveChanges();

    m_generalPage->ocrLanguageSelector()->saveSettings();
}

void SettingsDialog::updateWidgets()
{
    KConfigDialog::updateWidgets();
    m_shortcutsPage->resetChanges();

    m_generalPage->ocrLanguageSelector()->updateWidgets();
    m_generalPage->refreshOcrLanguageSettings();
}

void SettingsDialog::updateWidgetsDefault()
{
    KConfigDialog::updateWidgetsDefault();
    m_shortcutsPage->defaults();

    m_generalPage->ocrLanguageSelector()->applyDefaults();
    m_generalPage->refreshOcrLanguageSettings();
}

#include "moc_SettingsDialog.cpp"
