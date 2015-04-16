#include "CropScreenshotGrabber.h"

CropScreenshotGrabber::CropScreenshotGrabber(QObject *parent) :
    QObject(parent),
    mQuickView(nullptr),
    mKQmlObject(new KDeclarative::QmlObject)
{}

CropScreenshotGrabber::~CropScreenshotGrabber()
{
    if (mQuickView->visibility() != QQuickView::Hidden) {
        mQuickView->hide();
    }

    if (mQuickView) {
        delete mQuickView;
    }
    delete mKQmlObject;
}

void CropScreenshotGrabber::init()
{
    mQuickView = new QQuickView(mKQmlObject->engine(), 0);

    mQuickView->setResizeMode(QQuickView::SizeRootObjectToView);
    mQuickView->setFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Tool);
    mQuickView->setClearBeforeRendering(true);
    mQuickView->setColor(QColor(Qt::transparent));
    mQuickView->setSource(QUrl("qrc:///RegionGrabber.qml"));

    connect(mQuickView->engine(), SIGNAL(quit()), this, SIGNAL(selectionCancelled()));
    connect(mQuickView, &QQuickView::statusChanged, this, &CropScreenshotGrabber::waitForViewReady);
}

void CropScreenshotGrabber::waitForViewReady(QQuickView::Status status)
{
    switch(status) {
    case QQuickView::Ready: {
        QQuickItem *rootItem = mQuickView->rootObject();
        connect(rootItem, SIGNAL(selectionCancelled()), this, SIGNAL(selectionCancelled()));
        connect(rootItem, SIGNAL(selectionConfirmed(int,int,int,int)), this, SLOT(selectConfirmedHandler(int,int,int,int)));

        mQuickView->showFullScreen();
        return;
    }
    case QQuickView::Error:
        emit selectionCancelled();
        return;
    case QQuickView::Null:
    case QQuickView::Loading:
        return;
    }
}

void CropScreenshotGrabber::selectConfirmedHandler(int x, int y, int width, int height)
{
    mQuickView->hide();
    emit selectionConfirmed(x, y, width, height);
}
