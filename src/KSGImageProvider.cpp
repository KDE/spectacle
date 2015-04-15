#include "KSGImageProvider.h"

#include <QDebug>

KSGImageProvider::KSGImageProvider() :
    QQuickImageProvider(QQuickImageProvider::Pixmap),
    mPixmap(QPixmap())
{}

KSGImageProvider::~KSGImageProvider()
{}

QPixmap KSGImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(id);

    if (size) {
        *size = mPixmap.size();
    }

    if (requestedSize.isEmpty()) {
        return mPixmap;
    }
    return mPixmap.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void KSGImageProvider::setPixmap(const QPixmap &pixmap)
{
    mPixmap = pixmap;
}
