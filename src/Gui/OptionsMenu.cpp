/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "OptionsMenu.h"

#include "CaptureModeModel.h"
#include "Gui/SmartSpinBox.h"
#include "Gui/SettingsDialog/SettingsDialog.h"
#include "SpectacleCore.h"
#include "WidgetWindowUtils.h"
#include "settings.h"

#include <KLocalizedString>
#include <KStandardActions>

#include <QActionGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QStyle>
#include <QWidgetAction>

using namespace Qt::StringLiterals;

void OptionsMenu::ScreenshotActions::delayActionLayoutUpdate()
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

    int menuHMargin = q->style()->pixelMetric(QStyle::PM_MenuHMargin);
    int menuVMargin = q->style()->pixelMetric(QStyle::PM_MenuVMargin);
    if (q->layoutDirection() == Qt::RightToLeft) {
        delayLabel->setContentsMargins(0, 0, menuHMargin + delayLabel->fontMetrics().descent(), 0);
    } else {
        delayLabel->setContentsMargins(menuHMargin + delayLabel->fontMetrics().descent(), 0, 0, 0);
    }
    delayLayout->setContentsMargins(0, menuVMargin, 0, 0);
}

void OptionsMenu::ScreenshotActions::updateModes(QAction *before)
{
    auto model = CaptureModeModel::instance();
    captureModeActions.clear();
    for (int i = 0; i < model->rowCount(); ++i) {
        auto index = model->index(i);
        captureModeActions.emplace_back(new QAction(q));
        auto action = captureModeActions.back().get();
        action->setText(model->data(index, Qt::DisplayRole).toString());
        const auto mode = model->data(index, CaptureModeModel::CaptureModeRole).toInt();
        action->setData(mode);
        connect(action, &QAction::triggered, action, [mode] {
            SpectacleCore::instance()->takeNewScreenshot(mode);
        });
        q->insertAction(before, action);
    }
}

OptionsMenu::ScreenshotActions::ScreenshotActions(QAction *before, OptionsMenu *q)
    : q(q)
    , captureModeSection(new QAction(q))
    , captureSettingsSection(new QAction(q))
    , includeMousePointerAction(new QAction(q))
    , includeWindowDecorationsAction(new QAction(q))
    , includeWindowShadowAction(new QAction(q))
    , onlyCapturePopupAction(new QAction(q))
    , quitAfterSaveAction(new QAction(q))
    , captureOnClickSeparator(new QAction(q))
    , captureOnClickAction(new QAction(q))
    , delayAction(new QWidgetAction(q))
    , delayWidget(new QWidget(q))
    , delayLayout(new QHBoxLayout(delayWidget.get()))
    , delayLabel(new QLabel(delayWidget.get()))
    , delaySpinBox(new SmartSpinBox(delayWidget.get()))
{
    // QMenu::addSection just adds an action with text and separator mode enabled
    captureModeSection->setText(i18n("Capture Mode"));
    captureModeSection->setSeparator(true);
    q->insertAction(before, captureModeSection.get());

    captureSettingsSection->setText(i18n("Capture Settings"));
    captureSettingsSection->setSeparator(true);
    q->insertAction(before, captureSettingsSection.get());

    includeMousePointerAction->setText(i18n("Include mouse pointer"));
    includeMousePointerAction->setToolTip(i18n("Show the mouse cursor in the screenshot image"));
    includeMousePointerAction->setCheckable(true);
    includeMousePointerAction->setChecked(Settings::includePointer());
    QObject::connect(includeMousePointerAction.get(), &QAction::toggled, q, [](bool checked){
        Settings::setIncludePointer(checked);
    });
    QObject::connect(Settings::self(), &Settings::includePointerChanged, includeMousePointerAction.get(), [this](){
        includeMousePointerAction->setChecked(Settings::includePointer());
    });
    q->insertAction(before, includeMousePointerAction.get());

    includeWindowDecorationsAction->setText(i18n("Include window titlebar and borders"));
    includeWindowDecorationsAction->setToolTip(i18n("Show the window title bar, the minimize/maximize/close buttons, and the window border"));
    includeWindowDecorationsAction->setCheckable(true);
    includeWindowDecorationsAction->setChecked(Settings::includeDecorations());
    QObject::connect(includeWindowDecorationsAction.get(), &QAction::toggled, q, [](bool checked){
        Settings::setIncludeDecorations(checked);
    });
    QObject::connect(Settings::self(), &Settings::includeDecorationsChanged, includeWindowDecorationsAction.get(), [this](){
        includeWindowDecorationsAction->setChecked(Settings::includeDecorations());
    });
    q->insertAction(before, includeWindowDecorationsAction.get());

    includeWindowShadowAction->setText(i18n("Include window shadow"));
    includeWindowShadowAction->setToolTip(i18n("Show the window shadow"));
    includeWindowShadowAction->setCheckable(true);
    includeWindowShadowAction->setChecked(Settings::includeShadow());
    QObject::connect(includeWindowShadowAction.get(), &QAction::toggled, q, [](bool checked) {
        Settings::setIncludeShadow(checked);
    });
    QObject::connect(Settings::self(), &Settings::includeShadowChanged, includeWindowShadowAction.get(), [this]() {
        includeWindowShadowAction->setChecked(Settings::includeShadow());
    });
    q->insertAction(before, includeWindowShadowAction.get());

    onlyCapturePopupAction->setText(i18n("Capture the current pop-up only"));
    onlyCapturePopupAction->setToolTip(
        i18n("Capture only the current pop-up window (like a menu, tooltip etc).\n"
            "If disabled, the pop-up is captured along with the parent window"));
    onlyCapturePopupAction->setCheckable(true);
    onlyCapturePopupAction->setChecked(Settings::transientOnly());
    QObject::connect(onlyCapturePopupAction.get(), &QAction::toggled, q, [](bool checked){
        Settings::setTransientOnly(checked);
    });
    QObject::connect(Settings::self(), &Settings::transientOnlyChanged, onlyCapturePopupAction.get(), [this](){
        onlyCapturePopupAction->setChecked(Settings::transientOnly());
    });
    q->insertAction(before, onlyCapturePopupAction.get());

    quitAfterSaveAction->setText(i18n("Quit after manual Save or Copy"));
    quitAfterSaveAction->setToolTip(i18n("Quit Spectacle after manually saving or copying the image"));
    quitAfterSaveAction->setCheckable(true);
    quitAfterSaveAction->setChecked(Settings::quitAfterSaveCopyExport());
    QObject::connect(quitAfterSaveAction.get(), &QAction::toggled, q, [](bool checked){
        Settings::setQuitAfterSaveCopyExport(checked);
    });
    QObject::connect(Settings::self(), &Settings::quitAfterSaveCopyExportChanged, quitAfterSaveAction.get(), [this](){
        quitAfterSaveAction->setChecked(Settings::quitAfterSaveCopyExport());
    });
    q->insertAction(before, quitAfterSaveAction.get());

    // add capture on click
    const bool hasOnClick = SpectacleCore::instance()->imagePlatform()->supportedShutterModes().testFlag(ImagePlatform::OnClick);
    captureOnClickSeparator->setSeparator(true);
    captureOnClickSeparator->setVisible(hasOnClick);
    q->insertAction(before, captureOnClickSeparator.get());
    captureOnClickAction->setText(i18n("Capture On Click"));
    captureOnClickAction->setCheckable(true);
    captureOnClickAction->setChecked(Settings::captureOnClick() && hasOnClick);
    captureOnClickAction->setVisible(hasOnClick);
    QObject::connect(captureOnClickAction.get(), &QAction::toggled, q, [this](bool checked){
        Settings::setCaptureOnClick(checked);
        delayAction->setEnabled(!checked);
    });
    QObject::connect(Settings::self(), &Settings::captureOnClickChanged, captureOnClickAction.get(), [this](){
        captureOnClickAction->setChecked(Settings::captureOnClick());
    });
    q->insertAction(before, captureOnClickAction.get());

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
    QObject::connect(spinbox, qOverload<double>(&SmartSpinBox::valueChanged), q, [this](){
        if (updatingDelayActionLayout) {
            return;
        }
        Settings::setCaptureDelay(delaySpinBox->value());
    });
    QObject::connect(Settings::self(), &Settings::captureDelayChanged, spinbox, [this](){
        delaySpinBox->setValue(Settings::captureDelay());
    });
    delayWidget->setLayout(delayLayout.get());
    delayLayout->addWidget(label);
    delayLayout->addWidget(spinbox);
    delayLayout->setAlignment(Qt::AlignLeft);
    delayAction->setDefaultWidget(delayWidget.get());
    delayAction->setEnabled(!captureOnClickAction->isChecked());
    q->insertAction(before, delayAction.get());

    // Add capture mode actions.
    updateModes(captureSettingsSection.get());
}

void OptionsMenu::RecordingActions::updateModes(QAction *before)
{
    auto model = RecordingModeModel::instance();
    recordingModeActions.clear();
    for (int i = 0; i < model->rowCount(); ++i) {
        auto index = model->index(i);
        const auto mode = model->data(index, RecordingModeModel::RecordingModeRole).toInt();
        if (!CaptureWindow::instances().empty() && mode == VideoPlatform::Region) {
            continue;
        }
        recordingModeActions.emplace_back(new QAction(q));
        auto action = recordingModeActions.back().get();
        action->setText(model->data(index, Qt::DisplayRole).toString());
        action->setData(mode);
        connect(action, &QAction::triggered, action, [mode] {
            SpectacleCore::instance()->startRecording(VideoPlatform::RecordingMode(mode));
        });
        q->insertAction(before, action);
    }
}

OptionsMenu::RecordingActions::RecordingActions(QAction *before, OptionsMenu *q)
    : q(q)
    , recordingModeSection(new QAction(q))
    , recordingSettingsSection(new QAction(q))
    , includeMousePointerAction(new QAction(q))
{
    // QMenu::addSection just adds an action with text and separator mode enabled
    recordingModeSection->setText(i18n("Recording Mode"));
    recordingModeSection->setSeparator(true);
    q->insertAction(before, recordingModeSection.get());

    recordingSettingsSection->setText(i18n("Recording Settings"));
    recordingSettingsSection->setSeparator(true);
    q->insertAction(before, recordingSettingsSection.get());

    includeMousePointerAction->setText(i18n("Include mouse pointer"));
    includeMousePointerAction->setToolTip(i18n("Show the mouse cursor in the recording"));
    includeMousePointerAction->setCheckable(true);
    includeMousePointerAction->setChecked(Settings::videoIncludePointer());
    QObject::connect(includeMousePointerAction.get(), &QAction::toggled, q, [](bool checked){
        Settings::setVideoIncludePointer(checked);
    });
    QObject::connect(Settings::self(), &Settings::videoIncludePointerChanged, includeMousePointerAction.get(), [this](){
        includeMousePointerAction->setChecked(Settings::videoIncludePointer());
    });
    q->insertAction(before, includeMousePointerAction.get());

    // Add recording mode actions.
    updateModes(recordingSettingsSection.get());
}



static QPointer<OptionsMenu> s_instance = nullptr;

OptionsMenu::OptionsMenu(QWidget *parent)
    : SpectacleMenu(parent)
{
    auto separator = addSeparator();
    auto setActions = [separator, this] {
        const bool videoMode = SpectacleCore::instance()->videoMode();
        if (videoMode && !m_recordingActions) {
            m_screenshotActions.reset();
            m_recordingActions = std::make_unique<RecordingActions>(separator, this);
        } else if (!videoMode && !m_screenshotActions) {
            m_recordingActions.reset();
            m_screenshotActions = std::make_unique<ScreenshotActions>(separator, this);
        }
    };
    setActions();
    addAction(KStandardActions::preferences(this, &OptionsMenu::showPreferencesDialog, this));
    connect(SpectacleCore::instance(), &SpectacleCore::videoModeChanged, this, setActions);
    connect(RecordingModeModel::instance(), &RecordingModeModel::recordingModesChanged, this, [this] {
        if (m_recordingActions) {
            m_recordingActions->updateModes(m_recordingActions->recordingSettingsSection.get());
        }
    });
    connect(CaptureModeModel::instance(), &CaptureModeModel::captureModesChanged, this, [this] {
        if (m_screenshotActions) {
            m_screenshotActions->updateModes(m_screenshotActions->captureSettingsSection.get());
        }
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

void OptionsMenu::changeEvent(QEvent *event)
{
    switch(event->type()) {
    case QEvent::FontChange:
    case QEvent::LayoutDirectionChange:
    case QEvent::StyleChange: if (m_screenshotActions) {
        m_screenshotActions->delayActionLayoutUpdate();
        break;
    }
    default: break;
    }
    QWidget::changeEvent(event);
}

#include "moc_OptionsMenu.cpp"
