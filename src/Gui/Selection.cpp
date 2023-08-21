/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "Selection.h"
#include "SelectionEditor.h"

#include <QQuickItem>
#include <QtMath>

Selection::Selection(SelectionEditor *editor)
    : QObject(editor)
    , editor(editor)
{
}

qreal Selection::x() const
{
    return selection.x();
}

void Selection::setX(qreal x)
{
    QRectF newRect = selection;
    // QRectF::setX is the same as QRectF::setLeft
    newRect.moveLeft(x);
    setRect(newRect, Qt::Horizontal);
}

qreal Selection::y() const
{
    return selection.y();
}

void Selection::setY(qreal y)
{
    QRectF newRect = selection;
    // QRectF::setY is the same as QRectF::setTop
    newRect.moveTop(y);
    setRect(newRect, Qt::Vertical);
}

qreal Selection::width() const
{
    return selection.width();
}

void Selection::setWidth(qreal w)
{
    QRectF newRect = selection;
    newRect.setWidth(w);
    setRect(newRect.normalized(), Qt::Horizontal);
}

qreal Selection::height() const
{
    return selection.height();
}

void Selection::setHeight(qreal h)
{
    QRectF newRect = selection;
    newRect.setHeight(h);
    setRect(newRect.normalized(), Qt::Vertical);
}

qreal Selection::left() const
{
    return selection.left();
}

void Selection::setLeft(qreal l)
{
    QRectF newRect = selection;
    newRect.setLeft(l);
    setRect(newRect.normalized(), Qt::Horizontal);
}

qreal Selection::top() const
{
    return selection.top();
}

void Selection::setTop(qreal t)
{
    QRectF newRect = selection;
    newRect.setTop(t);
    setRect(newRect.normalized(), Qt::Vertical);
}

qreal Selection::right() const
{
    return selection.right();
}

void Selection::setRight(qreal r)
{
    QRectF newRect = selection;
    newRect.setRight(r);
    setRect(newRect.normalized(), Qt::Horizontal);
}

qreal Selection::bottom() const
{
    return selection.bottom();
}

void Selection::setBottom(qreal b)
{
    QRectF newRect = selection;
    newRect.setBottom(b);
    setRect(newRect.normalized(), Qt::Vertical);
}

qreal Selection::horizontalCenter() const
{
    return selection.x() + selection.width() / 2.0;
}

void Selection::setHorizontalCenter(qreal hc)
{
    qreal x = hc - selection.width() / 2.0;
    if (x == selection.x()) {
        return;
    }
    setX(x);
}

qreal Selection::verticalCenter() const
{
    return selection.y() + selection.height() / 2.0;
}

void Selection::setVerticalCenter(qreal vc)
{
    qreal y = vc - selection.height() / 2.0;
    if (y == selection.y()) {
        return;
    }
    setY(y);
}

void Selection::moveTo(qreal x, qreal y)
{
    QRectF newRect = selection;
    newRect.moveTo(x, y);
    setRect(newRect, Qt::Horizontal | Qt::Vertical);
}

void Selection::moveTo(const QPointF &p)
{
    QRectF newRect = selection;
    newRect.moveTo(p);
    setRect(newRect, Qt::Horizontal | Qt::Vertical);
}

void Selection::setRect(const QRectF &r)
{
    setRect(r.normalized(), Qt::Horizontal | Qt::Vertical);
}

void Selection::setRect(qreal x, qreal y, qreal w, qreal h)
{
    setRect(QRectF(x, y, w, h));
}

void Selection::setRect(const QRectF &newRect, Qt::Orientations orientations)
{
    const QRectF oldRect = selection;
    selection = newRect;
    // Using this instead of just comparing rects to take advantage
    // of the qFuzzyCompare calculations we're doing anyway.
    bool rectChange = false;
    // Keeping track of which things change without unnecessarily
    // sending signals or sending signals more than once is tough.
    if (orientations & Qt::Horizontal) {
        if (!qFuzzyCompare(oldRect.x(), newRect.x())) {
            rectChange = true;
            Q_EMIT xChanged();
        }
        qreal oldHC = oldRect.x() + oldRect.width() / 2.0;
        qreal newHC = newRect.x() + newRect.width() / 2.0;
        if (!qFuzzyCompare(oldHC, newHC)) {
            rectChange = true;
            Q_EMIT horizontalCenterChanged();
        }
        if (!qFuzzyCompare(oldRect.width(), newRect.width())) {
            rectChange = true;
            Q_EMIT widthChanged();
        }
        if (!qFuzzyCompare(oldRect.left(), newRect.left())) {
            rectChange = true;
            Q_EMIT leftChanged();
        }
        if (!qFuzzyCompare(oldRect.right(), newRect.right())) {
            rectChange = true;
            Q_EMIT rightChanged();
        }
    }
    if (orientations & Qt::Vertical) {
        if (!qFuzzyCompare(oldRect.y(), newRect.y())) {
            rectChange = true;
            Q_EMIT yChanged();
        }
        qreal oldVC = oldRect.y() + oldRect.height() / 2.0;
        qreal newVC = newRect.y() + newRect.height() / 2.0;
        if (!qFuzzyCompare(oldVC, newVC)) {
            rectChange = true;
            Q_EMIT verticalCenterChanged();
        }
        if (!qFuzzyCompare(oldRect.height(), newRect.height())) {
            rectChange = true;
            Q_EMIT heightChanged();
        }
        if (!qFuzzyCompare(oldRect.top(), newRect.top())) {
            rectChange = true;
            Q_EMIT topChanged();
        }
        if (!qFuzzyCompare(oldRect.bottom(), newRect.bottom())) {
            rectChange = true;
            Q_EMIT bottomChanged();
        }
    }
    if (rectChange) {
        Q_EMIT rectChanged();
    }
    if (oldRect.isEmpty() != newRect.isEmpty()) {
        Q_EMIT emptyChanged();
    }
}

QRectF Selection::rectF() const
{
    return selection;
}

QRect Selection::alignedRect(qreal dpr) const
{
    const int x = std::max(std::round(selection.x() * dpr), 0.0);
    const int y = std::max(std::round(selection.y() * dpr), 0.0);
    const int maxWidth = std::trunc(editor->width() * dpr);
    const int maxHeight = std::trunc(editor->height() * dpr);
    int width = std::round(selection.width() * dpr);
    int height = std::round(selection.height() * dpr);
    // prevent selection from going outside of selection editor
    const int widthDiff = x + width - maxWidth;
    const int heightDiff = y + height - maxHeight;
    if (widthDiff > 0) {
        width = width - widthDiff;
    }
    if (heightDiff > 0) {
        height = height - heightDiff;
    }
    return QRect(x, y, width, height);
}

QSizeF Selection::sizeF() const
{
    return selection.size();
}

QSize Selection::alignedSize(qreal width, qreal height, qreal dpr) const
{
    return QSize(std::min(std::round(width * dpr), std::trunc(editor->width() * dpr)),
                 std::min(std::round(height * dpr), std::trunc(editor->height() * dpr)));
}

QRectF Selection::normalized() const
{
    return selection.normalized();
}

bool Selection::isEmpty() const
{
    return selection.isEmpty();
}

bool Selection::contains(const QPointF &p) const
{
    return selection.contains(p);
}

bool Selection::rectContainsRect(const QRectF &rect1, const QRectF& rect2) const
{
    return rect1.contains(rect2);
}

bool Selection::rectIntersectsRect(const QRectF &rect1, const QRectF& rect2) const
{
    return rect1.intersects(rect2);
}

QDebug operator<<(QDebug debug, const Selection *selection)
{
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << selection->metaObject()->className() << '(';
    debug << selection->x() << ',' << selection->y() << ' '
          << selection->width() << 'x' << selection->height();
    const auto editor = qobject_cast<const SelectionEditor *>(selection->parent());
    qreal dpr = editor ? editor->devicePixelRatio() : 1;
    debug << " dpr=" << dpr;
    debug << ')';
    return debug;
}

#include "moc_Selection.cpp"
