#ifndef CROPSCREENSHOTGRABBER_H
#define CROPSCREENSHOTGRABBER_H

#include <QObject>
#include <QQuickView>
#include <QQuickItem>
#include <QQmlEngine>
#include <QUrl>

#include <KDeclarative/QmlObject>

class CropScreenshotGrabber : public QObject
{
    Q_OBJECT

    public:

    explicit CropScreenshotGrabber(QObject *parent = 0);
    ~CropScreenshotGrabber();

    void init();

    signals:

    void selectionCancelled();
    void selectionConfirmed(int x, int y, int width, int height);

    private slots:

    void waitForViewReady(QQuickView::Status status);
    void selectConfirmedHandler(int x, int y, int width, int height);

    private:

    QQuickView              *mQuickView;
    KDeclarative::QmlObject *mKQmlObject;
};

#endif // CROPSCREENSHOTGRABBER_H
