/*
 *  Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
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

#include "KSImageWidget.h"

KSImageWidget::KSImageWidget(QWidget *parent):
    QLabel(parent),
    mPixmap(QPixmap())
{
    mDSEffect = new QGraphicsDropShadowEffect(this);

    mDSEffect->setBlurRadius(SpectacleImage::SHADOW_RADIUS);
    mDSEffect->setOffset(0);
    mDSEffect->setColor(QColor(Qt::black));

    setGraphicsEffect(mDSEffect);
    setCursor(Qt::OpenHandCursor);
    setAlignment(Qt::AlignCenter);
    setMinimumSize(size());
}

void KSImageWidget::setScreenshot(const QPixmap &pixmap)
{
    mPixmap = pixmap;
    setToolTip(i18n("Image Size: %1x%2 pixels", mPixmap.width(), mPixmap.height()));
    setScaledPixmap();
}

void KSImageWidget::setScaledPixmap()
{
    const qreal scale = qApp->devicePixelRatio();
    QPixmap scaledPixmap = mPixmap.scaled(size() * scale, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    scaledPixmap.setDevicePixelRatio(scale);
    setPixmap(scaledPixmap);
}

// drag handlers

void KSImageWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        mDragStartPosition = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void KSImageWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        setCursor(Qt::OpenHandCursor);
    }
}

void KSImageWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }

    if ((event->pos() - mDragStartPosition).manhattanLength() < QGuiApplication::styleHints()->startDragDistance()) {
        return;
    }

    setCursor(Qt::OpenHandCursor);
    emit dragInitiated();
}

// resize handler

void KSImageWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    setScaledPixmap();
}

