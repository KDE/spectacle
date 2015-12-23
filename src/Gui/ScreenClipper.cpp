/*
 *   Copyright (C) 2007 Luca Gugelmann <lucag@student.ethz.ch>
 *   Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
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

#include "ScreenClipper.h"

ScreenClipper::ScreenClipper(const QPixmap &pixmap) :
    QRasterWindow(0),
    grabbing(false),
    mSelection(QRect()),
    mMouseOverHandle(0),
    mPixmap(pixmap)
{
    mTLHandle = mTRHandle = mBLHandle = mBRHandle = QRect(0, 0, 15, 15);
    mLHandle = mRHandle = QRect(0, 0, 10, 20);
    mTHandle = mBHandle = QRect(0, 0, 20, 10);
    mHandles = { &mTLHandle, &mTRHandle, &mBLHandle, &mBRHandle, &mLHandle, &mTHandle, &mRHandle, &mBHandle };

    setFlags(Qt::X11BypassWindowManagerHint | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Tool);
    QMetaObject::invokeMethod(this, "init", Qt::QueuedConnection);
}

ScreenClipper::~ScreenClipper()
{}

void ScreenClipper::init()
{
    mPixmap.setDevicePixelRatio(devicePixelRatio());
    setGeometry(0, 0, mPixmap.width(), mPixmap.height());
    setCursor(Qt::CrossCursor);
    showFullScreen();
}

inline void ScreenClipper::drawTriangle(QPainter *painter, const QColor &color, const QPoint &a, const QPoint &b, const QPoint &c)
{
    painter->save();

    QPainterPath path;
    path.moveTo(a);
    path.lineTo(b);
    path.lineTo(c);
    path.lineTo(a);

    painter->setPen(Qt::NoPen);
    painter->fillPath(path, QBrush(color));

    painter->restore();
}

void ScreenClipper::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);
    if (grabbing) {   // grabWindow() should just get the background
        return;
    }

    // start by initialising the QPainter and drawing the image on it

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.drawPixmap(0, 0, mPixmap);

    // set up the colors and fonts

    QPalette palette(QToolTip::palette());

    QColor textColor = palette.color(QPalette::Active, QPalette::Text);
    QColor textBackgroundColor = palette.color(QPalette::Active, QPalette::Base);
    QColor handleColor = palette.color(QPalette::Active, QPalette::Highlight);
    QColor overlayColor(0, 0, 0);

    overlayColor.setAlphaF(0.5);
    textBackgroundColor.setAlphaF(0.85);

    QFont font = QToolTip::font();
    font.setBold(true);
    font.setPointSize(10);
    painter.setFont(font);

    // if we don't have a selection yet, just draw a semitransparent
    // black rectangle over the whole screen, and render the help text

    const QRect normalizedWindowGeometry = QRect(geometry().x() / devicePixelRatio(),
                                                 geometry().y() / devicePixelRatio(),
                                                 geometry().width() / devicePixelRatio(),
                                                 geometry().height() / devicePixelRatio());

    if (mSelection.isNull() || mSelection.isEmpty()) {
        painter.setClipRegion(QRegion(normalizedWindowGeometry));
        painter.setPen(Qt::NoPen);
        painter.setBrush(overlayColor);
        painter.drawRect(normalizedWindowGeometry);

        painter.setPen(textColor);
        painter.setBrush(textBackgroundColor);

        QString helpText = i18n("Click anywhere on the screen (including on this text) to start drawing a selection rectangle, or press Esc to quit");
        QRect helpTextBoundingBox = painter.boundingRect(normalizedWindowGeometry, Qt::TextWordWrap, helpText);
        helpTextBoundingBox.moveCenter(normalizedWindowGeometry.center());
        QRect helpTextRect = helpTextBoundingBox.adjusted(-20, -20, 20, 20);

        painter.setPen(textColor);
        painter.setBrush(textBackgroundColor);
        painter.drawRoundedRect(helpTextRect, 10, 10);
        painter.drawText(helpTextBoundingBox, helpText);

        return;
    }

    // if we're here, this means we have a valid selection. let's draw
    // the overlay first

    QRegion region = QRegion(normalizedWindowGeometry).subtracted(mSelection);
    painter.setClipRegion(region);
    painter.setPen(Qt::NoPen);
    painter.setBrush(overlayColor);
    painter.drawRect(normalizedWindowGeometry);

    // and the selection rectangle border

    region = QRegion(mSelection).subtracted(mSelection.adjusted(1, 1, -1, -1));
    painter.setBrush(handleColor);
    painter.setClipRegion(region);
    painter.drawRect(mSelection);
    painter.setClipRect(normalizedWindowGeometry);

    // draw the handles

    updateHandles();
    if ((mSelection.height() > 20) && (mSelection.width() > 20)) {
        drawHandles(&painter, handleColor);
    }

    // render the help text

    painter.setPen(textColor);
    painter.setBrush(textBackgroundColor);

    QString helpText = i18n("To take the screenshot, double-click or press Enter. Right-click to reset the selection, or press Esc to quit.");
    QRect helpTextBoundingBox = painter.boundingRect(normalizedWindowGeometry, Qt::TextWordWrap, helpText);
    helpTextBoundingBox.moveCenter(normalizedWindowGeometry.center());
    helpTextBoundingBox.moveTop(normalizedWindowGeometry.top());
    QRect helpTextRect = helpTextBoundingBox.adjusted(-5, 0, 5, 10);
    helpTextBoundingBox.moveCenter(helpTextRect.center());

    painter.setPen(Qt::NoPen);
    painter.drawRect(helpTextRect);

    QPoint a = helpTextRect.topLeft();
    QPoint b = helpTextRect.bottomLeft() + QPoint(0, 1);
    QPoint c = QPoint(helpTextRect.left() - helpTextRect.height(), helpTextRect.top());
    drawTriangle(&painter, textBackgroundColor, a, b, c);

    a = helpTextRect.topRight() + QPoint(1, 0);
    b = helpTextRect.bottomRight() + QPoint(1, 1);
    c = QPoint(helpTextRect.right() + helpTextRect.height(), helpTextRect.top());
    drawTriangle(&painter, textBackgroundColor, a, b, c);

    painter.setPen(textColor);
    painter.drawText(helpTextBoundingBox, helpText);

}

void ScreenClipper::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e);

    if (mSelection.isNull()) {
        return;
    }

    QRect r = mSelection;
    r.setTopLeft(limitPointToRect(r.topLeft(), geometry()));
    r.setBottomRight(limitPointToRect(r.bottomRight(), geometry()));

    if (r.width() <= 1 || r.height() <= 1) {   // this just results in ugly drawing...
        mSelection = QRect();
    } else {
        mSelection = mSelection.normalized();
    }
}

void ScreenClipper::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        if (!mSelection.contains(e->pos())) {
            mSelection = QRect(e->pos(), e->pos());
            mMouseOverHandle = &mBRHandle;
        } else {
            mMoveDelta = e->pos() - mSelection.topLeft();
            setCursor(Qt::ClosedHandCursor);
        }
    } else if (e->button() == Qt::RightButton) {
        mSelection = QRect();
        setCursor(Qt::CrossCursor);
    }
    update();
}

void ScreenClipper::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons() & Qt::LeftButton) {
        if (mMouseOverHandle == nullptr) { // moving the whole selection
            QRect r = geometry();
            r.setBottomRight(r.bottomRight() - QPoint(mSelection.width(), mSelection.height()) + QPoint(1, 1));
            if (!(r.isNull() || r.isEmpty()) && r.isValid()) {
                const QPoint newTopLeft = limitPointToRect(e->pos() - mMoveDelta, r);
                if (newTopLeft == mSelection.topLeft()) {
                    mMoveDelta = e->pos() - mSelection.topLeft();
                } else {
                    mSelection.moveTo(newTopLeft);
                }
            }
        } else { // dragging a handle
            QRect r = mSelection;

            if (mMouseOverHandle == &mTLHandle) {
                if (e->pos().x() <= r.right() && e->pos().y() <= r.bottom()) {
                    r.setTopLeft(e->pos());
                } else if (e->pos().x() <= r.right() && e->pos().y() > r.bottom()) {
                    r.setLeft(e->pos().x());
                    r.setTop(r.bottom());
                    r.setBottom(e->pos().y());
                    mMouseOverHandle = &mBLHandle;
                } else if (e->pos().x() > r.right() && e->pos().y() <= r.bottom()) {
                    r.setTop(e->pos().y());
                    r.setLeft(r.right());
                    r.setRight(e->pos().x());
                    mMouseOverHandle = &mTRHandle;
                } else {
                    r.setTopLeft(r.bottomRight());
                    r.setBottomRight(e->pos());
                    mMouseOverHandle = &mBRHandle;
                }
                r = r.normalized();
            } else if (mMouseOverHandle == &mTRHandle) {
                if (e->pos().x() >= r.left() && e->pos().y() <= r.bottom()) {
                    r.setTopRight(e->pos());
                } else if (e->pos().x() >= r.left() && e->pos().y() > r.bottom()) {
                    r.setRight(e->pos().x());
                    r.setTop(r.bottom());
                    r.setBottom(e->pos().y());
                    mMouseOverHandle = &mBRHandle;
                } else if (e->pos().x() < r.left() && e->pos().y() <= r.bottom()) {
                    r.setTop(e->pos().y());
                    r.setRight(r.left());
                    r.setLeft(e->pos().x());
                    mMouseOverHandle = &mTLHandle;
                } else {
                    r.setTopRight(r.bottomLeft());
                    r.setBottomLeft(e->pos());
                    mMouseOverHandle = &mBLHandle;
                }
                r = r.normalized();
            } else if (mMouseOverHandle == &mBLHandle) {
                if (e->pos().x() <= r.right() && e->pos().y() >= r.top()) {
                    r.setBottomLeft(e->pos());
                } else if (e->pos().x() <= r.left() && e->pos().y() < r.top()) {
                    r.setLeft(e->pos().x());
                    r.setBottom(r.top());
                    r.setTop(e->pos().y());
                    mMouseOverHandle = &mTLHandle;
                } else if (e->pos().x() > r.left() && e->pos().y() >= r.top()) {
                    r.setBottom(e->pos().y());
                    r.setLeft(r.right());
                    r.setRight(e->pos().x());
                    mMouseOverHandle = &mBRHandle;
                } else {
                    r.setBottomLeft(r.topRight());
                    r.setTopRight(e->pos());
                    mMouseOverHandle = &mTRHandle;
                }
                r = r.normalized();
            } else if (mMouseOverHandle == &mBRHandle) {
                if (e->pos().x() >= r.left() && e->pos().y() >= r.top()) {
                    r.setBottomRight(e->pos());
                } else if (e->pos().x() >= r.left() && e->pos().y() < r.top()) {
                    r.setRight(e->pos().x());
                    r.setBottom(r.top());
                    r.setTop(e->pos().y());
                    mMouseOverHandle = &mTRHandle;
                } else if (e->pos().x() < r.left() && e->pos().y() >= r.top()) {
                    r.setBottom(e->pos().y());
                    r.setRight(r.left());
                    r.setLeft(e->pos().x());
                    mMouseOverHandle = &mBLHandle;
                } else {
                    r.setBottomRight(r.topLeft());
                    r.setTopLeft(e->pos());
                    mMouseOverHandle = &mTLHandle;
                }
                r = r.normalized();
            } else if (mMouseOverHandle == &mTHandle) {
                if (e->pos().y() <= r.bottom()) {
                    r.setTop(e->pos().y());
                } else {
                    r.setTop(r.bottom());
                    r.setBottom(e->pos().y());
                    mMouseOverHandle = &mBHandle;
                }
                r = r.normalized();
            } else if (mMouseOverHandle == &mRHandle) {
                if (e->pos().x() >= r.left()) {
                    r.setRight(e->pos().x());
                } else {
                    r.setRight(r.left());
                    r.setLeft(e->pos().x());
                    mMouseOverHandle = &mLHandle;
                }
                r = r.normalized();
            } else if (mMouseOverHandle == &mLHandle) {
                if (e->pos().x() <= r.right()) {
                    r.setLeft(e->pos().x());
                } else {
                    r.setLeft(r.right());
                    r.setRight(e->pos().x());
                    mMouseOverHandle = &mRHandle;
                }
                r = r.normalized();
            } else if (mMouseOverHandle == &mBHandle) {
                if (e->pos().y() >= r.top()) {
                    r.setBottom(e->pos().y());
                } else {
                    r.setBottom(r.top());
                    r.setTop(e->pos().y());
                    mMouseOverHandle = &mTHandle;
                }
                r = r.normalized();
            }

            mSelection = r.normalized();
        }

        update();
        return;
    }

    if (mSelection.isNull()) {
        return;
    }

    for (auto r: mHandles) {
        if (r->contains(e->pos())) {
            mMouseOverHandle = r;

            if (r == &mTLHandle || r == &mBRHandle) {
                setCursor(Qt::SizeFDiagCursor);
            } else if (r == &mTRHandle || r ==  &mBLHandle) {
                setCursor(Qt::SizeBDiagCursor);
            } else if (r == &mLHandle || r == &mRHandle) {
                setCursor(Qt::SizeHorCursor);
            } else if (r ==  &mTHandle || r == &mBHandle) {
                setCursor(Qt::SizeVerCursor);
            }

            return;
        }
    }

    mMouseOverHandle = nullptr;

    if (mSelection.contains(e->pos())) {
        setCursor(Qt::OpenHandCursor);
    } else {
        setCursor(Qt::CrossCursor);
    }
}

void ScreenClipper::mouseReleaseEvent(QMouseEvent *e)
{
    if (mMouseOverHandle == nullptr && mSelection.contains(e->pos())) {
        setCursor(Qt::OpenHandCursor);
    }
    update();
}

void ScreenClipper::mouseDoubleClickEvent(QMouseEvent *)
{
    return grabRect();
}

void ScreenClipper::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
        return grabRect();
    }

    if (e->key() == Qt::Key_Escape) {
        emit regionCancelled();
        return;
    }

    return e->ignore();
}

inline void ScreenClipper::grabRect()
{
    if (!mSelection.isNull() && mSelection.isValid()) {
        grabbing = true;

        const QRect normalizedSelection = QRect(mSelection.x() * devicePixelRatio(),
                                                mSelection.y() * devicePixelRatio(),
                                                mSelection.width() * devicePixelRatio(),
                                                mSelection.height() * devicePixelRatio());

        emit regionGrabbed(mPixmap.copy(normalizedSelection), normalizedSelection);
        return;
    }

    emit regionCancelled();
}

void ScreenClipper::updateHandles()
{
    QRect r = mSelection;

    mTLHandle.moveTopLeft(r.topLeft());
    mTRHandle.moveTopRight(r.topRight());
    mBLHandle.moveBottomLeft(r.bottomLeft());
    mBRHandle.moveBottomRight(r.bottomRight());

    mLHandle.moveTopLeft(QPoint(r.x(), r.y() + (r.height() / 2) - (mLHandle.height() / 2)));
    mTHandle.moveTopLeft(QPoint(r.x() + (r.width() / 2) - (mTHandle.width() / 2), r.y()));
    mRHandle.moveTopRight(QPoint(r.right(), r.y() + (r.height() / 2) - (mRHandle.height() / 2)));
    mBHandle.moveBottomLeft(QPoint(r.x() + (r.width() / 2) - (mBHandle.width() / 2), r.bottom()));
}

void ScreenClipper::drawHandles(QPainter *painter, const QColor &color)
{
    drawTriangle(painter, color, mTLHandle.topLeft(), mTLHandle.topRight(), mTLHandle.bottomLeft());
    drawTriangle(painter, color, mTRHandle.topRight(), mTRHandle.topLeft(), mTRHandle.bottomRight());
    drawTriangle(painter, color, mBLHandle.topLeft(), mBLHandle.bottomLeft(), mBLHandle.bottomRight());
    drawTriangle(painter, color, mBRHandle.topRight(), mBRHandle.bottomRight(), mBRHandle.bottomLeft());

    drawTriangle(painter, color, mTHandle.topLeft(), mTHandle.topRight(), (mTHandle.bottomLeft() + mTHandle.bottomRight()) / 2);
    drawTriangle(painter, color, mBHandle.bottomLeft(), mBHandle.bottomRight(), (mBHandle.topLeft() + mBHandle.topRight()) / 2);
    drawTriangle(painter, color, mLHandle.topLeft(), mLHandle.bottomLeft(), (mLHandle.topRight() + mLHandle.bottomRight()) / 2);
    drawTriangle(painter, color, mRHandle.topRight(), mRHandle.bottomRight(), (mRHandle.topLeft() + mRHandle.bottomLeft()) / 2);
}

QPoint ScreenClipper::limitPointToRect(const QPoint &p, const QRect &r) const
{
    QPoint q;
    q.setX(p.x() < r.x() ? r.x() : p.x() < r.right() ? p.x() : r.right());
    q.setY(p.y() < r.y() ? r.y() : p.y() < r.bottom() ? p.y() : r.bottom());
    return q;
}
