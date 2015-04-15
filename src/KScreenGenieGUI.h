#ifndef KSCREENGENIEGUI_H
#define KSCREENGENIEGUI_H

#include <QApplication>
#include <QMetaObject>
#include <QVariant>
#include <QList>
#include <QWidget>
#include <QQuickItem>
#include <QQuickWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMoveEvent>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QMenu>
#include <QPoint>
#include <QTimer>
#include <QDebug>
#include <QAction>
#include <QThread>

#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KAboutData>
#include <KHelpMenu>
#include <KGuiItem>
#include <KStandardGuiItem>
#include <KDeclarative/QmlObject>

#include "ImageGrabber.h"
#include "SendToActionsPopulator.h"
#include "KSGImageProvider.h"

class KScreenGenieGUI : public QWidget
{
    Q_OBJECT

    public:

    explicit KScreenGenieGUI(QWidget *parent = 0);
    ~KScreenGenieGUI();

    void setScreenshotAndShow(const QPixmap &pixmap);

    private slots:

    void addSendToAction(const QIcon icon, const QString name, const QVariant data);
    void addSendToSeperator();
    void captureScreenshot(QString captureMode, double captureDelay, bool includePointer, bool includeDecorations);
    void editScreenshot();
    void sendToRequest();
    void saveCheckboxStatesConfig(bool includePointer, bool includeDecorations);
    void saveCaptureModeConfig(int modeIndex);

    signals:

    void newScreenshotRequest(ImageGrabber::GrabMode mode, int timeout, bool includePointer, bool includeDecorations);
    void saveAndExit();
    void saveAsClicked();
    void sendToServiceRequest(KService::Ptr servicePointer);
    void sendToClipboardRequest();
    void sendToOpenWithRequest();

    protected:

    void moveEvent(QMoveEvent *event);

    private:

    void init();

    QQuickWidget            *mQuickWidget;
    QDialogButtonBox        *mDialogButtonBox;
    QPushButton             *mSendToButton;
    QMenu                   *mSendToMenu;
    KDeclarative::QmlObject *mKQmlObject;
    KSGImageProvider        *mScreenshotImageProvider;
    QList<QAction *>        mMenuActions;
};

#endif // KSCREENGENIEGUI_H
