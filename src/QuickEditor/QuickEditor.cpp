/*
 *  Copyright (C) 2016 Boudhayan Gupta <bgupta@kde.org>
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

#include "QuickEditor.h"

#include <QSize>
#include <QPixmap>
#include <QSharedPointer>
#include <QMetaObject>

#include <QQuickImageProvider>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QQuickView>
#include <QQmlEngine>

#include <KLocalizedString>
#include <KDeclarative/KDeclarative>

#include "SpectacleConfig.h"

struct QuickEditor::ImageStore : public QQuickImageProvider
{
    ImageStore(const QPixmap &pixmap) :
        QQuickImageProvider(QQuickImageProvider::Pixmap),
        mPixmap(pixmap)
    {}

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
    {
        Q_UNUSED(id);

        if (size) {
            *size = mPixmap.size();
        }

        if (requestedSize.isEmpty()) {
            return mPixmap;
        }

        return mPixmap.scaled(requestedSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    }

    QPixmap mPixmap;
};

struct QuickEditor::QuickEditorPrivate
{
    KDeclarative::KDeclarative *mDecl;
    QQuickView *mQuickView;
    QQmlEngine *mQmlEngine;
    QRect mGrabRect;
    QSharedPointer<QQuickItemGrabResult> mCurrentGrabResult;
};

QuickEditor::QuickEditor(const QPixmap &pixmap, QObject *parent) :
    QObject(parent),
    mImageStore(new ImageStore(pixmap)),
    d_ptr(new QuickEditorPrivate)
{
    Q_D(QuickEditor);

    d->mQmlEngine = new QQmlEngine();
    d->mDecl = new KDeclarative::KDeclarative;
    d->mDecl->setDeclarativeEngine(d->mQmlEngine);
    d->mDecl->setupBindings();
    d->mQmlEngine->addImageProvider(QStringLiteral("snapshot"), mImageStore);

    d->mQuickView = new QQuickView(d->mQmlEngine, 0);
    d->mQuickView->setSource(QUrl("qrc:///QuickEditor/EditorRoot.qml"));

    d->mQuickView->setFlags(Qt::BypassWindowManagerHint | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Tool);
    d->mQuickView->setGeometry(0, 0, pixmap.width(), pixmap.height());
    d->mQuickView->showFullScreen();

    // connect up the signals
    QQuickItem *rootItem = d->mQuickView->rootObject();
    connect(rootItem, SIGNAL(acceptImage(int, int, int, int)), this, SLOT(acceptImageHandler(int, int, int, int)));
    connect(rootItem, SIGNAL(cancelImage()), this, SIGNAL(grabCancelled()));

    // set up initial config
    SpectacleConfig *config = SpectacleConfig::instance();
    if (config->rememberLastRectangularRegion()) {
        QRect cropRegion = config->cropRegion();
        if (!cropRegion.isEmpty()) {
            QMetaObject::invokeMethod(
                rootItem, "setInitialSelection",
                Q_ARG(QVariant, cropRegion.x()),
                Q_ARG(QVariant, cropRegion.y()),
                Q_ARG(QVariant, cropRegion.width()),
                Q_ARG(QVariant, cropRegion.height())
            );
        }
    }

    if (config->useLightRegionMaskColour()) {
        rootItem->setProperty("maskColour", QColor(255, 255, 255, 192));
        rootItem->setProperty("strokeColour", QColor(96, 96, 96, 255));
    }
}

QuickEditor::~QuickEditor()
{
    Q_D(QuickEditor);
    delete d->mQuickView;
    delete d->mDecl;
    delete d->mQmlEngine;

    delete d_ptr;
}

void QuickEditor::acceptImageHandler(int x, int y, int width, int height)
{
    Q_D(QuickEditor);

    if ((x == -1) && (y == -1) && (width == -1) && (height == -1)) {
        SpectacleConfig::instance()->setCropRegion(QRect());
        emit grabCancelled();
        return;
    }

    auto pixelRatio = d->mQuickView->devicePixelRatio();
    d->mGrabRect = QRect(x * pixelRatio, y * pixelRatio, width * pixelRatio, height * pixelRatio);
    SpectacleConfig::instance()->setCropRegion(d->mGrabRect);

    d->mQuickView->hide();
    emit grabDone(mImageStore->mPixmap.copy(d->mGrabRect), d->mGrabRect);
}
