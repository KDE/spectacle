/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <qqml.h>
#include <QObject>
#include <QRectF>

class QQuickItem;
class SelectionEditor;

/**
 * This class provides information about the selected rectangle capture region and a few related utilities.
 */
class Selection : public QObject
{
    Q_OBJECT
    // TODO: make it impossible to misuse combinations of x/y/width/height,
    // left/top/right/bottom and horizontalCenter/verticalCenter bindings?
    Q_PROPERTY(qreal x READ x WRITE setX NOTIFY xChanged FINAL)
    Q_PROPERTY(qreal y READ y WRITE setY NOTIFY yChanged FINAL)
    Q_PROPERTY(qreal width READ width WRITE setWidth NOTIFY widthChanged FINAL)
    Q_PROPERTY(qreal height READ height WRITE setHeight NOTIFY heightChanged FINAL)

    Q_PROPERTY(qreal left READ left WRITE setLeft NOTIFY leftChanged FINAL)
    Q_PROPERTY(qreal top READ top WRITE setTop NOTIFY topChanged FINAL)
    Q_PROPERTY(qreal right READ right WRITE setRight NOTIFY rightChanged FINAL)
    Q_PROPERTY(qreal bottom READ bottom WRITE setBottom NOTIFY bottomChanged FINAL)

    Q_PROPERTY(qreal horizontalCenter READ horizontalCenter WRITE setHorizontalCenter NOTIFY horizontalCenterChanged FINAL)
    Q_PROPERTY(qreal verticalCenter READ verticalCenter WRITE setVerticalCenter NOTIFY verticalCenterChanged FINAL)

    Q_PROPERTY(QRectF rect READ rectF WRITE setRect NOTIFY rectChanged FINAL)

    Q_PROPERTY(bool empty READ isEmpty NOTIFY emptyChanged() FINAL)
    QML_ANONYMOUS

public:
    explicit Selection(SelectionEditor *editor);
    ~Selection() override = default;

    qreal x() const;
    void setX(qreal x);

    qreal y() const;
    void setY(qreal y);

    qreal width() const;
    void setWidth(qreal w);

    qreal height() const;
    void setHeight(qreal h);

    qreal left() const;
    void setLeft(qreal l);

    qreal top() const;
    void setTop(qreal t);

    qreal right() const;
    void setRight(qreal r);

    qreal bottom() const;
    void setBottom(qreal b);

    qreal horizontalCenter() const;
    void setHorizontalCenter(qreal hc);

    qreal verticalCenter() const;
    void setVerticalCenter(qreal vc);

    void moveTo(qreal x, qreal y);
    void moveTo(const QPointF &p);

    void setRect(const QRectF &r);
    void setRect(qreal x, qreal y, qreal w, qreal h);

    QRectF rectF() const;

    // The smallest QRect capable of fully containing the real rect
    // while also fitting inside of the SelectionEditor.
    // Optionally set the device pixel ratio.
    QRect alignedRect(qreal dpr = 1) const;

    QSizeF sizeF() const;

    // The smallest QSize capable of fully containing the real size
    // while also fitting inside of the SelectionEditor.
    // Optionally set the device pixel ratio.
    Q_INVOKABLE QSize alignedSize(qreal width, qreal height, qreal dpr = 1) const;

    QRectF normalized() const;

    bool isEmpty() const;

    bool contains(const QPointF &p) const;

    Q_INVOKABLE bool rectContainsRect(const QRectF &rect1, const QRectF& rect2) const;
    Q_INVOKABLE bool rectIntersectsRect(const QRectF &rect1, const QRectF& rect2) const;

Q_SIGNALS:
    void xChanged();
    void yChanged();
    void widthChanged();
    void heightChanged();

    void leftChanged();
    void topChanged();
    void rightChanged();
    void bottomChanged();

    void horizontalCenterChanged();
    void verticalCenterChanged();

    void rectChanged();

    void emptyChanged();

private:
    enum ChangeType {
        Horizontal = 0b01,
        Vertical =   0b10,
    };

    void setRect(const QRectF &newRect, int changeTypes);

    QRectF selection;
    // mainly exists so that I don't have to qobject_cast the parent
    SelectionEditor *const editor;
};

QDebug operator<<(QDebug debug, const Selection *selection);
