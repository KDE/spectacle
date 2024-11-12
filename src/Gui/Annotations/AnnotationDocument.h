/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "AnnotationTool.h"
#include "History.h"

#include <QColor>
#include <QFont>
#include <QImage>
#include <QMatrix4x4>
#include <QObject>
#include <QVariant>
#include <qqmlregistration.h>

class AnnotationTool;
class SelectedItemWrapper;
class QPainter;

/**
 * This class is used to render an image with annotations. The annotations are vector graphics
 * and image effects created from a stack of history items that can be undone or redone.
 * `paint()` and `renderToImage()` will be used by clients (e.g., AnnotationViewport) to render
 * their own content. There can be any amount of clients sharing the same AnnotationDocument.
 */
class AnnotationDocument : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by SpectacleCore")

    Q_PROPERTY(AnnotationTool *tool READ tool CONSTANT)
    Q_PROPERTY(SelectedItemWrapper *selectedItem READ selectedItemWrapper NOTIFY selectedItemWrapperChanged)

    Q_PROPERTY(int redoStackDepth READ redoStackDepth NOTIFY redoStackDepthChanged)
    Q_PROPERTY(int undoStackDepth READ undoStackDepth NOTIFY undoStackDepthChanged)
    Q_PROPERTY(QRectF canvasRect READ canvasRect NOTIFY canvasRectChanged)
    Q_PROPERTY(QSizeF imageSize READ imageSize NOTIFY imageSizeChanged)
    Q_PROPERTY(qreal imageDpr READ imageDpr NOTIFY imageDprChanged)

public:
    enum class ContinueOption {
        NoOptions    = 0b00,
        SnapAngle    = 0b01,
        CenterResize = 0b10
    };
    Q_DECLARE_FLAGS(ContinueOptions, ContinueOption)
    Q_FLAG(ContinueOption)

    enum class RepaintType {
        NoTypes = 0,
        BaseImage = 1,
        Annotations = 1 << 1,
        All = BaseImage | Annotations,
    };
    Q_DECLARE_FLAGS(RepaintTypes, RepaintType)
    Q_FLAG(RepaintType)

    explicit AnnotationDocument(QObject *parent = nullptr);
    ~AnnotationDocument();

    AnnotationTool *tool() const;
    SelectedItemWrapper *selectedItemWrapper() const;

    int undoStackDepth() const;
    int redoStackDepth() const;

    QRectF canvasRect() const;
    // Set the canvas rect, device pixel ratio, image size and reset the annotation image.
    // The image size and image device pixel ratio are also set based on these.
    void setCanvas(const QRectF &rect, qreal dpr);
    void resetCanvas();

    /// Image size in raw pixels
    QSizeF imageSize() const;

    /// Image device pixel ratio
    qreal imageDpr() const;

    QImage baseImage() const;
    // Get the base image section for the current canvas rect.
    QImage canvasBaseImage() const;
    /// Set the base image. Based on the base image, also set image size, image device pixel ratio
    // and canvas rect. Cannot be undone.
    void setBaseImage(const QImage &image);

    /// Hide annotations that do not intersect with the rectangle and crop the image.
    Q_INVOKABLE void cropCanvas(const QRectF &cropRect);

    /// Clear all annotations. Cannot be undone.
    void clearAnnotations();

    /// Clear all annotations and the image. Cannot be undone.
    void clear();

    // Paint the section of the image intersecting the viewport.
    void paintImageView(QPainter *painter, const QImage &image, const QRectF &viewport = {}) const;

    // Get an image containing just the annotations.
    // This is lazily computed based on an internal paint region of areas needing to be repainted.
    QImage annotationsImage();

    QImage renderToImage();

    // True when there is an item at the end of the undo stack and it is invalid.
    bool isCurrentItemValid() const;

    HistoryItem::shared_ptr popCurrentItem();

    // The first item with a mouse path intersecting the specified rectangle.
    // The rectangle is meant to be used as a way to make selecting an item more forgiving
    // by adding margins around the center of where the actual target point is.
    HistoryItem::const_shared_ptr itemAt(const QRectF &rect) const;

    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

    // For starting a new item
    void beginItem(const QPointF &point);
    void continueItem(const QPointF &point, AnnotationDocument::ContinueOptions options = ContinueOption::NoOptions);
    void finishItem();

    // For managing an existing item
    Q_INVOKABLE void selectItem(const QRectF &rect);
    Q_INVOKABLE void deselectItem();
    Q_INVOKABLE void deleteSelectedItem();

Q_SIGNALS:
    void selectedItemWrapperChanged();
    void undoStackDepthChanged();
    void redoStackDepthChanged();
    void canvasRectChanged();
    void imageSizeChanged();
    void imageDprChanged();

    void repaintNeeded(AnnotationDocument::RepaintTypes types);

private:
    friend class SelectedItemWrapper;

    // Paint the annotations intersecting the region.
    // The region is expected to be in image coordinates.
    // If the span is not set, all annotations intersecting the region will be painted.
    void paintAnnotations(QPainter *painter, const QRegion &imageRegion, std::optional<History::SubRange> range = std::nullopt) const;

    // Get an image that only uses a part of the history.
    QImage rangeImage(History::SubRange range) const;

    void addItem(const HistoryItem::shared_ptr &item);

    // Repaint if rect size is more than 0x0 and intersects with the canvas.
    // Takes a rectangle with document coordinates.
    // Defaults to Annotations because those are the most common.
    void setRepaintRegion(const QRectF &rect, RepaintTypes types = RepaintType::Annotations);
    // Unconditionally repaint. Defaults to All because that is most common for this function.
    void setRepaintRegion(RepaintTypes types = RepaintType::All);

    AnnotationTool *m_tool;
    SelectedItemWrapper *m_selectedItemWrapper;

    // The rectangle that contains the document area.
    QRectF m_canvasRect;
    // The device pixel ratio for the document's coordinate system.
    qreal m_imageDpr = 1;
    // An image size based on the canvas size and device pixel ratio.
    QSize m_imageSize{0, 0};
    // The base screenshot image
    QImage m_baseImage;
    // A cache for a crop of the base image.
    QImage m_croppedBaseImage;
    // An image containing just the annotations.
    // It is separate so that we don't need to keep repainting the image underneath.
    QImage m_annotationsImage;
    // The last types of things to repaint. Used to determine when to emit repaintNeeded.
    RepaintTypes m_lastRepaintTypes = RepaintType::NoTypes;
    // Where a repaint is needed. Used to determine when to repaint or emit repaintNeeded.
    QRegion m_repaintRegion;

    // A temporary version of the item we want to edit so we can modify at will. This will be used
    // instead of the original item when rendering, but the original item will remain in history
    // until the changes are committed.
    HistoryItem::shared_ptr m_tempItem;
    History m_history;
};

/**
 * When the user selects an existing shape with the mouse, this wraps all the parameters of the associated item, so that they can be modified from QML
 */
class SelectedItemWrapper : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by AnnotationDocument")

    Q_PROPERTY(bool hasSelection READ hasSelection CONSTANT)
    Q_PROPERTY(AnnotationTool::Options options READ options CONSTANT)
    Q_PROPERTY(int strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY strokeWidthChanged)
    Q_PROPERTY(QColor strokeColor READ strokeColor WRITE setStrokeColor NOTIFY strokeColorChanged)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged)
    Q_PROPERTY(qreal strength READ strength WRITE setStrength NOTIFY strengthChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(QColor fontColor READ fontColor WRITE setFontColor NOTIFY fontColorChanged)
    Q_PROPERTY(int number READ number WRITE setNumber NOTIFY numberChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(bool shadow READ hasShadow WRITE setShadow NOTIFY shadowChanged)
    Q_PROPERTY(QPainterPath mousePath READ mousePath NOTIFY mousePathChanged)

public:
    SelectedItemWrapper(AnnotationDocument *document);
    ~SelectedItemWrapper();

    // The item we are selecting.
    HistoryItem::const_weak_ptr selectedItem() const;
    void setSelectedItem(const HistoryItem::const_shared_ptr &item);

    // Transform the item with the given x and y deltas and at the specified edges.
    // Specifying no edges or all edges only translates.
    // We don't set things like scale directly because that would require more complex logic to be
    // written in various places in QML files.
    Q_INVOKABLE void transform(qreal dx, qreal dy, Qt::Edges edges = {});

    // Pushes the temporary item to history and sets the selected item as the temporary item parent.
    // Returns whether the commit actually happened.
    Q_INVOKABLE bool commitChanges();

    // Resets the selected item, temp item and options.
    bool reset();

    bool hasSelection() const;

    AnnotationTool::Options options() const;

    int strokeWidth() const;
    void setStrokeWidth(int width);

    QColor strokeColor() const;
    void setStrokeColor(const QColor &color);

    QColor fillColor() const;
    void setFillColor(const QColor &color);

    qreal strength() const;
    void setStrength(qreal strength);

    QFont font() const;
    void setFont(const QFont &font);

    QColor fontColor() const;
    void setFontColor(const QColor &color);

    int number() const;
    void setNumber(int number);

    QString text() const;
    void setText(const QString &text);

    bool hasShadow() const;
    void setShadow(bool shadow);

    QPainterPath mousePath() const;

Q_SIGNALS:
    void strokeWidthChanged();
    void strokeColorChanged();
    void fillColorChanged();
    void strengthChanged();
    void fontChanged();
    void fontColorChanged();
    void numberChanged();
    void textChanged();
    void shadowChanged();
    void mousePathChanged();

private:
    AnnotationTool::Options m_options;
    HistoryItem::const_weak_ptr m_selectedItem;
    AnnotationDocument *const m_document;
};

QDebug operator<<(QDebug debug, const SelectedItemWrapper *);

Q_DECLARE_OPERATORS_FOR_FLAGS(AnnotationDocument::ContinueOptions)
Q_DECLARE_OPERATORS_FOR_FLAGS(AnnotationDocument::RepaintTypes)
