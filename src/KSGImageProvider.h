#ifndef KSGIMAGEPROVIDER_H
#define KSGIMAGEPROVIDER_H

#include <QQuickImageProvider>
#include <QPixmap>
#include <QSize>
#include <QString>

class KSGImageProvider : public QQuickImageProvider
{
    public:

    explicit KSGImageProvider();
    ~KSGImageProvider();

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize);
    void setPixmap(const QPixmap &pixmap);

    private:

    QPixmap mPixmap;
};

#endif // KSGIMAGEPROVIDER_H
