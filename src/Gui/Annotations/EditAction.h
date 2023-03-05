/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "AnnotationDocument.h"
#include <QColor>
#include <QFont>
#include <QObject>
#include <QPainterPath>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>

/**
 * This is the base for all edit actions. A document is defined as a stack of instances of its subclasses,
 * which defines what to draw at a given step. The Undo functionality is done by removing actions from the stack
 */
class EditAction
{
public:
    virtual ~EditAction();

    AnnotationDocument::EditActionType type() const;

    int strokeWidth() const;
    void setStrokeWidth(int width);

    QColor strokeColor() const;
    void setStrokeColor(const QColor &color);

    QColor fillColor() const;
    void setFillColor(const QColor &color);

    QFont font() const;
    void setFont(const QFont &font);

    QColor fontColor() const;
    void setFontColor(const QColor &color);

    int number() const;
    void setNumber(int number);

    QMarginsF strokeMargins() const;
    QMarginsF shadowMargins() const;

    QRectF lastUpdateArea() const;

    virtual bool isValid() const = 0;
    bool isVisible() const;

    /**
     * If an old action (like a rectangle) was selected and then modified (rectangle resized, colors changed etc)
     * a new action of the same type will be created on top of the stack, marking this one as replaced by the other one.
     * When the user undoes the action that replaces this one, this old action gets visible again, showing the
     * rectangle before the changes.
     */
    EditAction *replacedBy() const;

    /**
     * If this is the *new* action, this points to the old action this releases
     */
    EditAction *replaces() const;

    /**
     * Creates a copy of this action, excluding `replaces` and `replacedBy`.
     */
    EditAction *createCopy();

    /**
     * Creates a copy that replaces this action.
     */
    EditAction *createReplacement();

    /**
     * Copies all properties from another action to this action,
     * excluding `replaces` and `replacedBy`.
     * Will not work for actions of different types.
     */
    void copyFrom(EditAction *other);

    void translate(const QPointF &delta);

    bool supportsShadow() const;
    bool hasShadow() const;
    void setShadow(bool shadow);

    virtual QRectF geometry() const = 0;
    virtual void setGeometry(const QRectF &geom) = 0;

    virtual QRectF visualGeometry() const;
    virtual void setVisualGeometry(const QRectF &geom);

    /**
     * Checks if an action is equivalent to this action, excluding `replaces` and `replacedBy`.
     */
    bool operator==(const EditAction *other) const;
    bool operator!=(const EditAction *other) const;

protected:
    explicit EditAction(AnnotationTool *tool);
    explicit EditAction(EditAction *action);

    virtual QRectF getUpdateArea() const;

    AnnotationDocument::EditActionType m_type = AnnotationDocument::None;
    int m_strokeWidth = 0;
    QColor m_strokeColor;
    QColor m_fillColor;
    QFont m_font;
    QColor m_fontColor;
    int m_number = 0;
    QRectF m_lastUpdateArea;
    bool m_supportsShadow = false;
    bool m_hasShadow = false;
    EditAction *m_replacedBy = nullptr;
    EditAction *m_replaces = nullptr;

private:
    friend class DeleteAction;
};

QDebug operator<<(QDebug debug, const EditAction *action);

// This action can "delete" an object of any action it replaces
class DeleteAction : public EditAction
{
public:
    DeleteAction(AnnotationTool *tool) = delete;
    DeleteAction(EditAction *replaced);
    ~DeleteAction();

    bool isValid() const override;

    void setGeometry(const QRectF &geom) override;
    QRectF geometry() const override;
};

class FreeHandAction : public EditAction
{
public:
    FreeHandAction(AnnotationTool *tool, const QPointF &startPoint);
    ~FreeHandAction();

    QPainterPath path() const;
    void addPoint(const QPointF &point);

    bool isValid() const override;

    void makeSmooth();

    void setGeometry(const QRectF &geom) override;
    QRectF geometry() const override;
    QRectF visualGeometry() const override;
    void setVisualGeometry(const QRectF &geom) override;

private:
    FreeHandAction(FreeHandAction *action);

    QPainterPath m_path;

    friend class EditAction;
};

class LineAction : public EditAction
{
public:
    LineAction(AnnotationTool *tool, const QPointF &startPoint);
    ~LineAction();

    QLineF line() const;
    void setEndPoint(const QPointF &endPoint);
    QPolygonF arrowHeadPolygon(const QLineF &mainLine) const;

    bool isValid() const override;

    void setGeometry(const QRectF &geom) override;
    QRectF geometry() const override;
    QRectF visualGeometry() const override;
    void setVisualGeometry(const QRectF &geom) override;

protected:
    QRectF getUpdateArea() const override;

private:
    LineAction(LineAction *action);

    QLineF m_line;

    friend class EditAction;
};

class ShapeAction : public EditAction
{
public:
    ShapeAction(AnnotationTool *tool, const QPointF &startPoint);
    ~ShapeAction();

    bool isValid() const override;

    void setGeometry(const QRectF &geom) override;
    QRectF geometry() const override;
    QRectF visualGeometry() const override;
    void setVisualGeometry(const QRectF &geom) override;

    QImage &backingStoreCache();
    void invalidateCache();

private:
    ShapeAction(ShapeAction *action);

    QRectF m_rect;
    QImage m_backingStoreCache;

    friend class EditAction;
};

class TextAction : public EditAction
{
public:
    TextAction(AnnotationTool *tool, const QPointF &startPoint);
    ~TextAction();

    QPointF startPoint() const;
    QString text() const;
    void setText(const QString &text);

    bool isValid() const override;

    void setGeometry(const QRectF &geom) override;
    QRectF geometry() const override;

private:
    TextAction(TextAction *action);

    QRectF m_boundingRect;
    QString m_text;

    friend class EditAction;
};

class NumberAction : public EditAction
{
public:
    NumberAction(AnnotationTool *tool, const QPointF &startPoint);
    ~NumberAction();

    QPointF startPoint() const;
    QRectF boundingRect() const;

    int padding() const;

    bool isValid() const override;

    void setGeometry(const QRectF &geom) override;
    QRectF geometry() const override;

private:
    NumberAction(NumberAction *action);

    QRectF m_boundingRect;
    int m_padding = 5;

    friend class EditAction;
};
