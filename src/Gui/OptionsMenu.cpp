/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "OptionsMenu.h"

#include "CaptureModeModel.h"
#include "Gui/SettingsDialog/SettingsDialog.h"
#include "SpectacleCore.h"
#include "WidgetWindowUtils.h"
#include "settings.h"

#include <KLocalizedString>
#include <KStandardActions>

#include <QStyle>

using namespace Qt::StringLiterals;

static QPointer<OptionsMenu> s_instance = nullptr;

OptionsMenu::OptionsMenu(QWidget *parent)
    : SpectacleMenu(parent)
    , captureModeSection(new QAction(this))
    , captureModeGroup(new QActionGroup(this)) // exclusive by default)
    , captureSettingsSection(new QAction(this))
    , includeMousePointerAction(new QAction(this))
    , includeWindowDecorationsAction(new QAction(this))
    , includeWindowShadowAction(new QAction(this))
    , onlyCapturePopupAction(new QAction(this))
    , quitAfterSaveAction(new QAction(this))
    , captureOnClickAction(new QAction(this))
    , delayAction(new QWidgetAction(this))
    , delayWidget(new QWidget(this))
    , delayLayout(new QHBoxLayout(delayWidget.get()))
    , delayLabel(new QLabel(delayWidget.get()))
    , delaySpinBox(new SmartSpinBox(delayWidget.get()))
{
    addAction(KStandardActions::preferences(this, &OptionsMenu::showPreferencesDialog, this));

    // QMenu::addSection just adds an action with text and separator mode enabled
    captureModeSection->setText(i18n("Capture Mode"));
    captureModeSection->setSeparator(true);
    addAction(captureModeSection.get());

    // Add capture mode actions.
    // This cannot be done in the constructor because captureModeModel will be null at this time.
    connect(this, &OptionsMenu::aboutToShow,
            this, &OptionsMenu::updateCaptureModes);

    // make capture mode actions do things
    connect(captureModeGroup.get(), &QActionGroup::triggered, this, [](QAction *action){
        int mode = action->data().toInt();
        Settings::setCaptureMode(mode);
    });
    connect(Settings::self(), &Settings::captureModeChanged, this, [this](){
        int mode = Settings::captureMode();
        if (captureModeGroup->checkedAction() && mode == captureModeGroup->checkedAction()->data().toInt()) {
            return;
        }
        for (auto action : std::as_const(captureModeActions)) {
            if (mode == action->data().toInt()) {
                action->setChecked(true);
            }
        }
    });

    captureSettingsSection->setText(i18n("Capture Settings"));
    captureSettingsSection->setSeparator(true);
    addAction(captureSettingsSection.get());

    includeMousePointerAction->setText(i18n("Include mouse pointer"));
    includeMousePointerAction->setToolTip(i18n("Show the mouse cursor in the screenshot image"));
    includeMousePointerAction->setCheckable(true);
    includeMousePointerAction->setChecked(Settings::includePointer());
    connect(includeMousePointerAction.get(), &QAction::toggled, this, [](bool checked){
        Settings::setIncludePointer(checked);
    });
    connect(Settings::self(), &Settings::includePointerChanged, this, [this](){
        includeMousePointerAction->setChecked(Settings::includePointer());
    });
    addAction(includeMousePointerAction.get());

    includeWindowDecorationsAction->setText(i18n("Include window titlebar and borders"));
    includeWindowDecorationsAction->setToolTip(i18n("Show the window title bar, the minimize/maximize/close buttons, and the window border"));
    includeWindowDecorationsAction->setCheckable(true);
    includeWindowDecorationsAction->setChecked(Settings::includeDecorations());
    connect(includeWindowDecorationsAction.get(), &QAction::toggled, this, [](bool checked){
        Settings::setIncludeDecorations(checked);
    });
    connect(Settings::self(), &Settings::includeDecorationsChanged, this, [this](){
        includeWindowDecorationsAction->setChecked(Settings::includeDecorations());
    });
    addAction(includeWindowDecorationsAction.get());

    includeWindowShadowAction->setText(i18n("Include window shadow"));
    includeWindowShadowAction->setToolTip(i18n("Show the window shadow"));
    includeWindowShadowAction->setCheckable(true);
    includeWindowShadowAction->setChecked(Settings::includeShadow());
    connect(includeWindowShadowAction.get(), &QAction::toggled, this, [](bool checked) {
        Settings::setIncludeShadow(checked);
    });
    connect(Settings::self(), &Settings::includeShadowChanged, this, [this]() {
        includeWindowShadowAction->setChecked(Settings::includeShadow());
    });
    addAction(includeWindowShadowAction.get());

    onlyCapturePopupAction->setText(i18n("Capture the current pop-up only"));
    onlyCapturePopupAction->setToolTip(
        i18n("Capture only the current pop-up window (like a menu, tooltip etc).\n"
             "If disabled, the pop-up is captured along with the parent window"));
    onlyCapturePopupAction->setCheckable(true);
    onlyCapturePopupAction->setChecked(Settings::transientOnly());
    connect(onlyCapturePopupAction.get(), &QAction::toggled, this, [](bool checked){
        Settings::setTransientOnly(checked);
    });
    connect(Settings::self(), &Settings::transientOnlyChanged, this, [this](){
        onlyCapturePopupAction->setChecked(Settings::transientOnly());
    });
    addAction(onlyCapturePopupAction.get());

    quitAfterSaveAction->setText(i18n("Quit after manual Save or Copy"));
    quitAfterSaveAction->setToolTip(i18n("Quit Spectacle after manually saving or copying the image"));
    quitAfterSaveAction->setCheckable(true);
    quitAfterSaveAction->setChecked(Settings::quitAfterSaveCopyExport());
    connect(quitAfterSaveAction.get(), &QAction::toggled, this, [](bool checked){
        Settings::setQuitAfterSaveCopyExport(checked);
    });
    connect(Settings::self(), &Settings::quitAfterSaveCopyExportChanged, this, [this](){
        quitAfterSaveAction->setChecked(Settings::quitAfterSaveCopyExport());
    });
    addAction(quitAfterSaveAction.get());

    addSeparator();

    // add capture on click
    const bool hasOnClick = SpectacleCore::instance()->imagePlatform()->supportedShutterModes().testFlag(ImagePlatform::OnClick);
    addSeparator()->setVisible(hasOnClick);
    captureOnClickAction->setText(i18n("Capture On Click"));
    captureOnClickAction->setCheckable(true);
    captureOnClickAction->setChecked(Settings::captureOnClick() && hasOnClick);
    captureOnClickAction->setVisible(hasOnClick);
    connect(captureOnClickAction.get(), &QAction::toggled, this, [this](bool checked){
        Settings::setCaptureOnClick(checked);
        delayAction->setEnabled(!checked);
    });
    connect(Settings::self(), &Settings::captureOnClickChanged, this, [this](){
        captureOnClickAction->setChecked(Settings::captureOnClick());
    });
    addAction(captureOnClickAction.get());

    // set up delay widget
    auto spinbox = delaySpinBox.get();
    auto label = delayLabel.get();
    label->setText(i18n("Delay:"));
    spinbox->setDecimals(1);
    spinbox->setSingleStep(1.0);
    spinbox->setMinimum(0.0);
    spinbox->setMaximum(999);
    spinbox->setSpecialValueText(i18n("No Delay"));
    delayActionLayoutUpdate();
    connect(spinbox, qOverload<double>(&SmartSpinBox::valueChanged), this, [this](){
        if (updatingDelayActionLayout) {
            return;
        }
        Settings::setCaptureDelay(delaySpinBox->value());
    });
    connect(Settings::self(), &Settings::captureDelayChanged, this, [this](){
        delaySpinBox->setValue(Settings::captureDelay());
    });
    delayWidget->setLayout(delayLayout.get());
    delayLayout->addWidget(label);
    delayLayout->addWidget(spinbox);
    delayLayout->setAlignment(Qt::AlignLeft);
    delayAction->setDefaultWidget(delayWidget.get());
    delayAction->setEnabled(!captureOnClickAction->isChecked());
    addAction(delayAction.get());
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

void OptionsMenu::setCaptureModeOptionsEnabled(bool enabled)
{
    captureModeOptionsEnabled = enabled;
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

void OptionsMenu::delayActionLayoutUpdate()
{
    // We can't block signals while doing this to prevent unnecessary
    // processing because the spinbox has internal connections that need
    // to work in order to get the correct size.
    // We use our own guarding variable instead.
    updatingDelayActionLayout = true;
    delaySpinBox->setValue(delaySpinBox->maximum());
    delaySpinBox->setMinimumWidth(delaySpinBox->sizeHint().width());
    delaySpinBox->setValue(Settings::captureDelay());
    updatingDelayActionLayout = false;

    int menuHMargin = style()->pixelMetric(QStyle::PM_MenuHMargin);
    int menuVMargin = style()->pixelMetric(QStyle::PM_MenuVMargin);
    if (layoutDirection() == Qt::RightToLeft) {
        delayLabel->setContentsMargins(0, 0, menuHMargin + delayLabel->fontMetrics().descent(), 0);
    } else {
        delayLabel->setContentsMargins(menuHMargin + delayLabel->fontMetrics().descent(), 0, 0, 0);
    }
    delayLayout->setContentsMargins(0, menuVMargin, 0, 0);
}

void OptionsMenu::updateCaptureModes()
{
    captureModeSection->setVisible(captureModeOptionsEnabled);
    if (!captureModeOptionsEnabled) {
        for (auto action : std::as_const(captureModeActions)) {
            captureModeGroup->removeAction(action);
            removeAction(action);
            action->deleteLater();
        }
        captureModeActions.clear();
        return;
    }
    auto captureModeModel = SpectacleCore::instance()->captureModeModel();
    if (captureModeModel == nullptr) {
        qWarning() << Q_FUNC_INFO << "captureModeModel is null, not updating actions";
        return;
    }
    // Only make this conneciton once.
    // Can't be done in the constructor because captureModeModel is null at that time.
    if (!captureModesInitialized) {
        connect(captureModeModel, &CaptureModeModel::captureModesChanged, this, [this](){
            shouldUpdateCaptureModes = true;
        });
        captureModesInitialized = true;
    }
    // avoid unnecessarily resetting actions
    if (!shouldUpdateCaptureModes) {
        return;
    }
    shouldUpdateCaptureModes = false;
    for (auto action : std::as_const(captureModeActions)) {
        captureModeGroup->removeAction(action);
        removeAction(action);
        action->deleteLater();
    }
    captureModeActions.clear();
    for (int i = 0; i < captureModeModel->rowCount(); ++i) {
        auto index = captureModeModel->index(i);
        auto action = new QAction(this);
        captureModeActions.append(action);
        action->setText(captureModeModel->data(index, Qt::DisplayRole).toString());
        const auto mode = captureModeModel->data(index, CaptureModeModel::CaptureModeRole).toInt();
        action->setData(mode);
        action->setCheckable(true);
        if (!CaptureWindow::instances().empty() && !SpectacleCore::instance()->videoMode()) {
            action->setChecked(mode == CaptureModeModel::RectangularRegion);
        } else if (mode == Settings::captureMode()) {
            action->setChecked(true);
        }
        captureModeGroup->addAction(action);
        insertAction(captureSettingsSection.get(), action);
    }
}

#include "moc_OptionsMenu.cpp"
