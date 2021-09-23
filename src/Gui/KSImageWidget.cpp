/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KSImageWidget.h"
#include <KLocalizedString>
#include <QGuiApplication>
#include <QStyleHints>

KSImageWidget::KSImageWidget(QWidget *parent)
    : QLabel(parent)
    , mPixmap(QPixmap())
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
    Q_EMIT dragInitiated();
}

// resize handler

void KSImageWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    setScaledPixmap();
}
