/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QLabel>
#include <QMouseEvent>
#include <QPoint>
#include <QPixmap>
#include <QGraphicsDropShadowEffect>


namespace SpectacleImage {
    static const int SHADOW_RADIUS = 5;
}

class KSImageWidget : public QLabel
{
    Q_OBJECT


    public:

    explicit KSImageWidget(QWidget *parent = nullptr);
    void setScreenshot(const QPixmap &pixmap);

    Q_SIGNALS:

    void dragInitiated();

    protected:

    void mousePressEvent(QMouseEvent *event)   override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event)    override;
    void resizeEvent(QResizeEvent *event)      override;

    private:

    void setScaledPixmap();

    QGraphicsDropShadowEffect *mDSEffect;
    QPixmap                    mPixmap;
    QPoint                     mDragStartPosition;
};
