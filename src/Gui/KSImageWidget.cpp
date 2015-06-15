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

#include "KSImageWidget.h"

KSImageWidget::KSImageWidget(QWidget *parent):
    QLabel(parent)
{
    mDSEffect = new QGraphicsDropShadowEffect(this);

    mDSEffect->setBlurRadius(5);
    mDSEffect->setOffset(0);
    mDSEffect->setColor(QColor(Qt::black));

    setGraphicsEffect(mDSEffect);
    setCursor(Qt::OpenHandCursor);
    setAlignment(Qt::AlignCenter);
}

void KSImageWidget::setScreenshot(const QPixmap &pixmap)
{
    QPixmap pix = pixmap.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    setPixmap(pix);
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

