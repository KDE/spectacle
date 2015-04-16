#ifndef CROPSCREENSHOTGRABBER_H
#define CROPSCREENSHOTGRABBER_H

#include <QObject>
#include <QQuickView>
#include <QQuickItem>
#include <QQmlEngine>
#include <QUrl>
#include <QPixmap>
#include <QMetaObject>

#include <KDeclarative/QmlObject>

#include "KSGImageProvider.h"

class CropScreenshotGrabber : public QObject
{
    Q_OBJECT

    public:

    explicit CropScreenshotGrabber(bool liveMode = true, QObject *parent = 0);
    ~CropScreenshotGrabber();

    void init(QPixmap pixmap = QPixmap());

    signals:

    void selectionCancelled();
    void selectionConfirmed(int x, int y, int width, int height);

    private slots:

    void waitForViewReady(QQuickView::Status status);
    void selectConfirmedHandler(int x, int y, int width, int height);

    private:

    QQuickView              *mQuickView;
    KDeclarative::QmlObject *mKQmlObject;
    KSGImageProvider        *mImageProvider;
    bool                    mLiveMode;
};

#endif // CROPSCREENSHOTGRABBER_H
