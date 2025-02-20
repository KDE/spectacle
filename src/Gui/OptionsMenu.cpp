/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "OptionsMenu.h"

#include "CaptureModeModel.h"
#include "Gui/SmartSpinBox.h"
#include "Gui/SettingsDialog/SettingsDialog.h"
#include "SpectacleCore.h"
#include "WidgetWindowUtils.h"
#include "HelpMenu.h"
#include "settings.h"

#include <KLocalizedString>
#include <KStandardActions>

#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QStyle>
#include <QWidgetAction>

using namespace Qt::StringLiterals;

static QPointer<OptionsMenu> s_instance = nullptr;

OptionsMenu::OptionsMenu(QWidget *parent)
    : SpectacleMenu(parent)
    , m_delayAction(new QWidgetAction(this))
    , m_delayWidget(new QWidget(this))
    , m_delayLayout(new QHBoxLayout(m_delayWidget.get()))
    , m_delayLabel(new QLabel(m_delayWidget.get()))
    , m_delaySpinBox(new SmartSpinBox(m_delayWidget.get()))
{
    setToolTipsVisible(true);
    // QMenu::addSection just adds an action with text and separator mode enabled
    addSection(i18nc("@title:menu", "Screenshot Settings"));

    auto includeMousePointerAction = addAction(i18nc("@option:check for screenshots", "Include mouse pointer"));
    includeMousePointerAction->setToolTip(i18nc("@info:tooltip", "Show the mouse cursor in the screenshot image"));
    includeMousePointerAction->setCheckable(true);
    includeMousePointerAction->setChecked(Settings::includePointer());
    QObject::connect(includeMousePointerAction, &QAction::toggled, Settings::self(), &Settings::setIncludePointer);
    QObject::connect(Settings::self(), &Settings::includePointerChanged, includeMousePointerAction, [includeMousePointerAction](){
        includeMousePointerAction->setChecked(Settings::includePointer());
    });

    auto includeWindowDecorationsAction = addAction(i18nc("@option:check", "Include window titlebar and borders"));
    includeWindowDecorationsAction->setToolTip(i18nc("@info:tooltip", "Show the window title bar, the minimize/maximize/close buttons, and the window border"));
    includeWindowDecorationsAction->setCheckable(true);
    includeWindowDecorationsAction->setChecked(Settings::includeDecorations());
    QObject::connect(includeWindowDecorationsAction, &QAction::toggled, Settings::self(), Settings::setIncludeDecorations);
    QObject::connect(Settings::self(), &Settings::includeDecorationsChanged, includeWindowDecorationsAction, [includeWindowDecorationsAction](){
        includeWindowDecorationsAction->setChecked(Settings::includeDecorations());
    });

    auto includeWindowShadowAction = addAction(i18nc("@option:check", "Include window shadow"));
    includeWindowShadowAction->setToolTip(i18nc("@info:tooltip", "Show the window shadow"));
    includeWindowShadowAction->setCheckable(true);
    includeWindowShadowAction->setChecked(Settings::includeShadow());
    QObject::connect(includeWindowShadowAction, &QAction::toggled, Settings::self(), &Settings::setIncludeShadow);
    QObject::connect(Settings::self(), &Settings::includeShadowChanged, includeWindowShadowAction, [includeWindowShadowAction]() {
        includeWindowShadowAction->setChecked(Settings::includeShadow());
    });

    const bool hasTransientWithParent = SpectacleCore::instance()->imagePlatform()->supportedGrabModes().testFlag(ImagePlatform::TransientWithParent);
    if (hasTransientWithParent) {
        auto onlyCapturePopupAction = addAction(i18nc("@option:check", "Capture the current pop-up only"));
        onlyCapturePopupAction->setToolTip(
            i18nc("@info:tooltip", "Capture only the current pop-up window (like a menu, tooltip etc).\n"
                "If disabled, the pop-up is captured along with the parent window"));
        onlyCapturePopupAction->setCheckable(true);
        onlyCapturePopupAction->setChecked(Settings::transientOnly());
        QObject::connect(onlyCapturePopupAction, &QAction::toggled, Settings::self(), &Settings::setTransientOnly);
        QObject::connect(Settings::self(), &Settings::transientOnlyChanged, onlyCapturePopupAction, [onlyCapturePopupAction](){
            onlyCapturePopupAction->setChecked(Settings::transientOnly());
        });
    }

    auto quitAfterSaveAction = addAction(i18nc("@option:check", "Quit after manual Save or Copy"));
    quitAfterSaveAction->setToolTip(i18nc("@info:tooltip", "Quit Spectacle after manually saving or copying the image"));
    quitAfterSaveAction->setCheckable(true);
    quitAfterSaveAction->setChecked(Settings::quitAfterSaveCopyExport());
    QObject::connect(quitAfterSaveAction, &QAction::toggled, Settings::self(), &Settings::setQuitAfterSaveCopyExport);
    QObject::connect(Settings::self(), &Settings::quitAfterSaveCopyExportChanged, quitAfterSaveAction, [quitAfterSaveAction](){
        quitAfterSaveAction->setChecked(Settings::quitAfterSaveCopyExport());
    });

    // add capture on click
    const bool hasOnClick = SpectacleCore::instance()->imagePlatform()->supportedShutterModes().testFlag(ImagePlatform::OnClick);
    if (hasOnClick) {
        addSeparator();
        auto captureOnClickAction = addAction(i18nc("@option:check", "Capture On Click"));
        captureOnClickAction->setCheckable(true);
        captureOnClickAction->setChecked(Settings::captureOnClick());
        QObject::connect(captureOnClickAction, &QAction::toggled, this, [this](bool checked){
            Settings::setCaptureOnClick(checked);
            m_delayAction->setEnabled(!checked);
        });
        QObject::connect(Settings::self(), &Settings::captureOnClickChanged, captureOnClickAction, [captureOnClickAction](){
            captureOnClickAction->setChecked(Settings::captureOnClick());
        });
    }

    // set up delay widget
    auto spinbox = m_delaySpinBox.get();
    auto label = m_delayLabel.get();
    label->setText(i18nc("@label:spinbox", "Delay:"));
    spinbox->setDecimals(1);
    spinbox->setSingleStep(1.0);
    spinbox->setMinimum(0.0);
    spinbox->setMaximum(999);
    spinbox->setSpecialValueText(i18nc("@item 0 delay special value", "No Delay"));
    delayActionLayoutUpdate();
    QObject::connect(spinbox, qOverload<double>(&SmartSpinBox::valueChanged), this, [this](){
        if (m_updatingDelayActionLayout) {
            return;
        }
        Settings::setCaptureDelay(m_delaySpinBox->value());
    });
    QObject::connect(Settings::self(), &Settings::captureDelayChanged, spinbox, [this](){
        m_delaySpinBox->setValue(Settings::captureDelay());
    });
    m_delayWidget->setLayout(m_delayLayout.get());
    m_delayLayout->addWidget(label);
    m_delayLayout->addWidget(spinbox);
    m_delayLayout->setAlignment(Qt::AlignLeft);
    m_delayAction->setDefaultWidget(m_delayWidget.get());
    m_delayAction->setEnabled(!hasOnClick || !Settings::captureOnClick());
    addAction(m_delayAction.get());

    addSection(i18nc("@title:menu", "Recording Settings"));

    auto videoIncludeMousePointerAction = addAction(i18nc("@option:check for recordings", "Include mouse pointer"));
    videoIncludeMousePointerAction->setToolTip(i18nc("@info:tooltip", "Show the mouse cursor in the recording"));
    videoIncludeMousePointerAction->setCheckable(true);
    videoIncludeMousePointerAction->setChecked(Settings::videoIncludePointer());
    QObject::connect(videoIncludeMousePointerAction, &QAction::toggled, Settings::self(), &Settings::setVideoIncludePointer);
    QObject::connect(Settings::self(), &Settings::videoIncludePointerChanged, videoIncludeMousePointerAction, [videoIncludeMousePointerAction](){
        videoIncludeMousePointerAction->setChecked(Settings::videoIncludePointer());
    });

    addSeparator();

    addAction(KStandardActions::preferences(this, &OptionsMenu::showPreferencesDialog, this));

    addMenu(HelpMenu::instance());
    connect(this, &OptionsMenu::aboutToShow,
            this, [this] {
                setWidgetTransientParentToWidget(HelpMenu::instance(), this);
            });
}

OptionsMenu *OptionsMenu::instance()
{
    // We need to create it here instead of using Q_GLOBAL_STATIC like the other menus
    // to prevent a segfault.
    if (!s_instance) {
        s_instance = new OptionsMenu;
    }
    return s_instance;
}

void OptionsMenu::showPreferencesDialog()
{
    KConfigDialog *dialog = KConfigDialog::exists(u"settings"_s);
    if (!dialog) {
        dialog = new SettingsDialog;
        dialog->setAttribute(Qt::WA_DeleteOnClose);
    }

    // properly set the transientparent chain
    setWidgetTransientParent(dialog, getWidgetTransientParent(this));

    // HACK: how to make it appear on top of the fullscreen window? dialog->setWindowFlags(Qt::Popup); is an ugly way

    dialog->show();
}

void OptionsMenu::delayActionLayoutUpdate()
{
    // We can't block signals while doing this to prevent unnecessary
    // processing because the spinbox has internal connections that need
    // to work in order to get the correct size.
    // We use our own guarding variable instead.
    m_updatingDelayActionLayout = true;
    m_delaySpinBox->setValue(m_delaySpinBox->maximum());
    m_delaySpinBox->setMinimumWidth(m_delaySpinBox->sizeHint().width());
    m_delaySpinBox->setValue(Settings::captureDelay());
    m_updatingDelayActionLayout = false;

    int menuHMargin = style()->pixelMetric(QStyle::PM_MenuHMargin);
    int menuVMargin = style()->pixelMetric(QStyle::PM_MenuVMargin);
    if (layoutDirection() == Qt::RightToLeft) {
        m_delayLabel->setContentsMargins(0, 0, menuHMargin + m_delayLabel->fontMetrics().descent(), 0);
    } else {
        m_delayLabel->setContentsMargins(menuHMargin + m_delayLabel->fontMetrics().descent(), 0, 0, 0);
    }
    m_delayLayout->setContentsMargins(0, menuVMargin, 0, 0);
}

void OptionsMenu::changeEvent(QEvent *event)
{
    switch(event->type()) {
    case QEvent::FontChange:
    case QEvent::LayoutDirectionChange:
    case QEvent::StyleChange:
        delayActionLayoutUpdate();
        break;
    default: break;
    }
    QWidget::changeEvent(event);
}

void OptionsMenu::keyPressEvent(QKeyEvent *event)
{
    // Try to keep menu open when triggering checkable actions
    const auto key = event->key();
    const auto action = activeAction();
    if (action && action->isEnabled() && action->isCheckable() //
        && (key == Qt::Key_Return || key == Qt::Key_Enter //
            || (key == Qt::Key_Space && style()->styleHint(QStyle::SH_Menu_SpaceActivatesItem, nullptr, this)))) {
        action->trigger();
        event->accept();
        return;
    }
    SpectacleMenu::keyPressEvent(event);
}

void OptionsMenu::mouseReleaseEvent(QMouseEvent *event)
{
    // Try to keep menu open when triggering checkable actions
    const auto action = activeAction() == actionAt(event->position().toPoint()) ? activeAction() : nullptr;
    if (action && action->isEnabled() && action->isCheckable()) {
        action->trigger();
        return;
    }
    SpectacleMenu::mouseReleaseEvent(event);
}

#include "moc_OptionsMenu.cpp"
