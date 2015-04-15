#include "KScreenGenieGUI.h"

KScreenGenieGUI::KScreenGenieGUI(QWidget *parent) :
    QWidget(parent),
    mQuickWidget(nullptr),
    mDialogButtonBox(nullptr),
    mSendToButton(nullptr),
    mSendToMenu(new QMenu),
    mKQmlObject(new KDeclarative::QmlObject),
    mScreenshotImageProvider(new KSGImageProvider)
{
    QTimer::singleShot(10, this, &KScreenGenieGUI::init);
}

KScreenGenieGUI::~KScreenGenieGUI()
{
    if (mQuickWidget) {
        delete mQuickWidget;
        delete mKQmlObject;
    }
}

// GUI init

void KScreenGenieGUI::init()
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kscreengenierc");
    KConfigGroup guiConfig(config, "GuiConfig");

    // window properties

    setWindowTitle(i18nc("Untitled Screenshot", "Untitled"));
    setFixedSize(800, 370);

    QPoint location = guiConfig.readEntry("window-position", QPoint(50, 50));
    move(location);

    // the QtQuick widget

    mKQmlObject->engine()->addImageProvider(QLatin1String("screenshot"), mScreenshotImageProvider);
    mQuickWidget = new QQuickWidget(mKQmlObject->engine(), this);

    mQuickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    mQuickWidget->resize(mQuickWidget->width(), 300);
    mQuickWidget->setClearColor(QWidget::palette().color(QWidget::backgroundRole()));
    mQuickWidget->setSource(QUrl("qrc:///MainForm.qml"));

    // connect the the qml signals

    QQuickItem *rootItem = mQuickWidget->rootObject();
    connect(rootItem, SIGNAL(newScreenshotRequest(QString,double,bool,bool)), this, SLOT(captureScreenshot(QString,double,bool,bool)));
    connect(rootItem, SIGNAL(editScreenshotRequest()), this, SLOT(editScreenshot()));
    connect(rootItem, SIGNAL(saveCheckboxStates(bool, bool)), this, SLOT(saveCheckboxStatesConfig(bool, bool)));
    connect(rootItem, SIGNAL(saveCaptureMode(int)), this, SLOT(saveCaptureModeConfig(int)));

    // the Button Bar

    mDialogButtonBox = new QDialogButtonBox(this);
    mDialogButtonBox->setStandardButtons(QDialogButtonBox::Help | QDialogButtonBox::Discard | QDialogButtonBox::Apply | QDialogButtonBox::Save);

    mSendToButton = new QPushButton;
    KGuiItem::assign(mSendToButton, KGuiItem(i18n("Send To...")));
    mDialogButtonBox->addButton(mSendToButton, QDialogButtonBox::ActionRole);

    connect(mDialogButtonBox->button(QDialogButtonBox::Discard), &QPushButton::clicked, qApp, &QApplication::quit);
    connect(mDialogButtonBox->button(QDialogButtonBox::Save), &QPushButton::clicked, this, &KScreenGenieGUI::saveAsClicked);
    KGuiItem::assign(mDialogButtonBox->button(QDialogButtonBox::Save), KStandardGuiItem::saveAs());

    connect(mDialogButtonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &KScreenGenieGUI::saveAndExit);
    KGuiItem::assign(mDialogButtonBox->button(QDialogButtonBox::Apply), KStandardGuiItem::save());
    mDialogButtonBox->button(QDialogButtonBox::Apply)->setText(i18n("Save && Exit"));
    mDialogButtonBox->button(QDialogButtonBox::Apply)->setToolTip(i18n("Quicksave screenshot in your Pictures directory and exit"));

    KHelpMenu *helpMenu = new KHelpMenu(this, KAboutData::applicationData(), true);
    mDialogButtonBox->button(QDialogButtonBox::Help)->setMenu(helpMenu->menu());

    // layouts

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mQuickWidget);
    layout->addWidget(mDialogButtonBox);
    setLayout(layout);
    mQuickWidget->setFocus();

    // start a thread to populate our send-to actions

    QThread *thread = new QThread;
    SendToActionsPopulator *populator = new SendToActionsPopulator;

    populator->moveToThread(thread);
    connect(thread, &QThread::started, populator, &SendToActionsPopulator::process);
    connect(populator, &SendToActionsPopulator::haveAction, this, &KScreenGenieGUI::addSendToAction);
    connect(populator, &SendToActionsPopulator::haveSeperator, this, &KScreenGenieGUI::addSendToSeperator);
    //connect(populator, &SendToActionsPopulator::allDone, thread, &QThread::quit);
    //connect(populator, &SendToActionsPopulator::allDone, thread, &QThread::deleteLater);

    mSendToButton->setMenu(mSendToMenu);
    thread->start();

    // read in the checkbox states and capture mode index

    bool includePointer = guiConfig.readEntry("includePointer", true);
    bool includeDecorations = guiConfig.readEntry("includeDecorations", true);
    QMetaObject::invokeMethod(rootItem, "loadCheckboxStates", Q_ARG(QVariant, includePointer), Q_ARG(QVariant, includeDecorations));

    int captureModeIndex = guiConfig.readEntry("captureModeIndex", 0);
    QMetaObject::invokeMethod(rootItem, "loadCaptureMode", Q_ARG(QVariant, captureModeIndex));

    // done with the init
}

// gui events

void KScreenGenieGUI::moveEvent(QMoveEvent *event)
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kscreengenierc");
    KConfigGroup guiConfig(config, "GuiConfig");

    guiConfig.writeEntry("window-position", event->pos());
    guiConfig.sync();
}

// slots

void KScreenGenieGUI::addSendToAction(const QIcon icon, const QString name, const QVariant data)
{
    QAction *action = new QAction(icon, name, nullptr);
    action->setData(data);
    connect(action, &QAction::triggered, this, &KScreenGenieGUI::sendToRequest);

    mMenuActions.append(action);
    mSendToMenu->addAction(action);
}

void KScreenGenieGUI::addSendToSeperator()
{
    mSendToMenu->addSeparator();
}

void KScreenGenieGUI::captureScreenshot(QString captureMode, double captureDelay, bool includePointer, bool includeDecorations)
{
    hide();

    ImageGrabber::GrabMode mode = ImageGrabber::InvalidChoice;
    int msec = captureDelay * 1000;;

    if (captureMode == QStringLiteral("fullScreen")) {
        mode = ImageGrabber::FullScreen;
    } else if (captureMode == QStringLiteral("currentScreen")) {
        mode = ImageGrabber::CurrentScreen;
    } else if (captureMode == QStringLiteral("activeWindow")) {
        mode = ImageGrabber::ActiveWindow;
    } else if (captureMode == QStringLiteral("rectangularRegion")) {
        mode = ImageGrabber::RectangularRegion;
    } else {
        qWarning() << "Capture called with invalid mode";
        show();
        return;
    }

    emit newScreenshotRequest(mode, msec, includePointer, includeDecorations);
}

void KScreenGenieGUI::editScreenshot()
{
    qDebug() << "Edit called";
}

void KScreenGenieGUI::setScreenshotAndShow(const QPixmap &pixmap)
{
    mScreenshotImageProvider->setPixmap(pixmap);

    QQuickItem *rootItem = mQuickWidget->rootObject();
    QMetaObject::invokeMethod(rootItem, "reloadScreenshot");

    show();
}

void KScreenGenieGUI::sendToRequest()
{
    QAction *action = qobject_cast<QAction *>(QObject::sender());
    if (!(action)) {
        qWarning() << "Internal qobject_cast error. This is a bug.";
        return;
    }

    KService::Ptr servicePointer;

    const ActionData data = action->data().value<ActionData>();
    switch(data.first) {
    case SendToActionsPopulator::HardcodedAction:
        if (data.second == "clipboard") {
            emit sendToClipboardRequest();
        } else if (data.second == "application") {
            emit sendToOpenWithRequest();
        }
        return;
    case SendToActionsPopulator::KServiceAction:
        servicePointer = KService::serviceByMenuId(data.second);
        emit sendToServiceRequest(servicePointer);
        return;
    case SendToActionsPopulator::KipiAction:
        qDebug() << "KIPI";
        return;
    }
}

void KScreenGenieGUI::saveCheckboxStatesConfig(bool includePointer, bool includeDecorations)
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kscreengenierc");
    KConfigGroup guiConfig(config, "GuiConfig");

    guiConfig.writeEntry("includePointer", includePointer);
    guiConfig.writeEntry("includeDecorations", includeDecorations);
    guiConfig.sync();
}

void KScreenGenieGUI::saveCaptureModeConfig(int modeIndex)
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kscreengenierc");
    KConfigGroup guiConfig(config, "GuiConfig");

    guiConfig.writeEntry("captureModeIndex", modeIndex);
    guiConfig.sync();
}
