/*
 *   Copyright (C) 2007 Luca Gugelmann <lucag@student.ethz.ch>
 *   Copyright (C) 2015 Boudhayan Gupta <me@BaloneyGeek.com>
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SCREENCLIPPER_H
#define SCREENCLIPPER_H

#include <QRasterWindow>
#include <QRegion>
#include <QPoint>
#include <QVector>
#include <QRect>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

#include <KLocalizedString>
#include <KWindowSystem>

class ScreenClipper : public QRasterWindow
{
    Q_OBJECT

    public:

    ScreenClipper(const QPixmap &pixmap = QPixmap());
    ~ScreenClipper();

    protected slots:

    void init();

    signals:

    void regionGrabbed(const QPixmap &pixmap, const QRect &region);
    void regionCancelled();

    protected:

    void paintEvent(QPaintEvent *e) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *e) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;

    private:

    void updateHandles();
    void drawHandles(QPainter *painter, const QColor &color);
    void drawTriangle(QPainter *painter, const QColor &color, const QPoint &a, const QPoint &b, const QPoint &c);
    void grabRect();
    QPoint limitPointToRect(const QPoint &p, const QRect &r) const;

    bool grabbing;

    // naming convention for handles
    // T top, B bottom, R Right, L left
    // 2 letters: a corner
    // 1 letter: the handle on the middle of the corresponding side
    QRect  mTLHandle, mTRHandle, mBLHandle, mBRHandle;
    QRect  mLHandle, mTHandle, mRHandle, mBHandle;
    QVector<QRect *> mHandles;
    QRect  mSelection;
    QRect *mMouseOverHandle;
    QPoint mMoveDelta;

    QPixmap mPixmap;
};

#endif

