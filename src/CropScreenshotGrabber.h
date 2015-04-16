/*
 *  Copyright (C) 2015 Boudhayan Gupta <me@BaloneyGeek.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

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
