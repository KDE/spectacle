/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QColor>
#include <QFont>
#include <QImage>
#include <QObject>
#include <QVariant>

class AnnotationTool;
class SelectedActionWrapper;
class EditAction;
class QPainter;

/**
 * This is the base logic that defines a Document: it's basically a stack of EditAction instances
 * which will decide how to actually render the document: there will be no QImages in the document until saved.
 * is more akin to a Vector drawin app. The undo functionality is implemented by removing EditActions form the top of
 * m_undoStack and moving them to m_redoStack.
 * paint and REnderToImage will be used by clients (AnnotationViewport) to render their own content.
 * There can be any amount of AnnotationViewport sharing the same AnnotationDocument.
 */
class AnnotationDocument : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AnnotationTool *tool READ tool CONSTANT)
    Q_PROPERTY(SelectedActionWrapper *selectedAction READ selectedActionWrapper NOTIFY selectedActionWrapperChanged)

    Q_PROPERTY(int redoStackDepth READ redoStackDepth NOTIFY redoStackDepthChanged)
    Q_PROPERTY(int undoStackDepth READ undoStackDepth NOTIFY undoStackDepthChanged)
    Q_PROPERTY(QSizeF canvasSize READ canvasSize NOTIFY canvasSizeChanged)
    Q_PROPERTY(QSizeF imageSize READ imageSize NOTIFY imageSizeChanged)
    Q_PROPERTY(qreal imageDpr READ imageDpr NOTIFY imageDprChanged)

public:
    enum EditActionType { None, FreeHand, Highlight, Line, Arrow, Rectangle, Ellipse, Blur, Pixelate, Text, Number, ChangeAction };
    Q_ENUM(EditActionType)

    enum class ContinueOption {
        NoOptions    = 0b00,
        SnapAngle    = 0b01,
        CenterResize = 0b10
    };
    Q_DECLARE_FLAGS(ContinueOptions, ContinueOption)
    Q_FLAG(ContinueOption)

    enum class RenderOption {
        // No RenderNone because that's pointless
        Images      = 0b01,
        Annotations = 0b10,
        RenderAll   = Images | Annotations
    };
    Q_DECLARE_FLAGS(RenderOptions, RenderOption)
    Q_FLAG(RenderOption)

    explicit AnnotationDocument(QObject *parent = nullptr);
    ~AnnotationDocument();

    AnnotationTool *tool() const;
    SelectedActionWrapper *selectedActionWrapper() const;

    int undoStackDepth() const;
    int redoStackDepth() const;

    void paint(QPainter *painter, const QRectF &viewPort, qreal zoomFactor = 1.0, RenderOptions options = RenderOption::RenderAll) const;
    QImage renderToImage(const QRectF &viewPort, qreal scale = 1, RenderOptions options = RenderOption::RenderAll) const;
    QImage renderToImage() const;

    // Actions that can't be undone
    void clear();
    void cropCanvas(const QRectF &cropRect);
    QSizeF canvasSize() const;

    void setImage(const QImage &image);

    void clearAnnotations();

    /**
     * Image size in raw pixels
     */
    QSizeF imageSize() const;

    qreal imageDpr() const;

    // True when there is an edit action in the undo stack and it is invalid.
    Q_INVOKABLE bool isLastActionInvalid() const;

    Q_INVOKABLE void permanentlyDeleteLastAction();

    EditAction *actionAtPoint(const QPointF &point) const;

    Q_INVOKABLE QRectF visualGeometryAtPoint(const QPointF &point) const;

public Q_SLOTS:
    void undo();
    void redo();
    void beginAction(const QPointF &point);
    void continueAction(const QPointF &point, AnnotationDocument::ContinueOptions options = ContinueOption::NoOptions);
    void finishAction();
    void selectAction(const QPointF &point);
    void deselectAction();
    void deleteSelectedAction();

Q_SIGNALS:
    void selectedActionWrapperChanged();
    void undoStackDepthChanged();
    void redoStackDepthChanged();
    void canvasSizeChanged();
    void imageSizeChanged();
    void imageDprChanged();

    void repaintNeeded(const QRectF &area = {});

private:
    friend class SelectedActionWrapper;

    void addAction(EditAction *action);
    void permanentlyDeleteAction(EditAction *action);
    void clearRedoStack();
    void emitRepaintNeededUnlessEmpty(const QRectF &area);

    bool isActionVisible(EditAction *action, const QRectF &rect = {}) const;

    AnnotationTool *m_tool;
    SelectedActionWrapper *m_selectedActionWrapper;

    QRectF m_canvasRect;
    QImage m_image;
    QVector<EditAction *> m_undoStack;
    QVector<EditAction *> m_redoStack;
};

/**
 * This is the data structure that controls the creation of the next action. From qml its paramenter will be
 * set by the app toolbars, and then drawing on the screen with the mouse will lead to the creation of
 * a new EditAction based on those parameters
 */
class AnnotationTool : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AnnotationDocument::EditActionType type READ type WRITE setType RESET resetType NOTIFY typeChanged)
    Q_PROPERTY(Options options READ options NOTIFY optionsChanged)
    Q_PROPERTY(int strokeWidth READ strokeWidth WRITE setStrokeWidth RESET resetStrokeWidth NOTIFY strokeWidthChanged)
    Q_PROPERTY(QColor strokeColor READ strokeColor WRITE setStrokeColor RESET resetStrokeColor NOTIFY strokeColorChanged)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor RESET resetFillColor NOTIFY fillColorChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont RESET resetFont NOTIFY fontChanged)
    Q_PROPERTY(QColor fontColor READ fontColor WRITE setFontColor RESET resetFontColor NOTIFY fontColorChanged)
    Q_PROPERTY(int number READ number WRITE setNumber RESET resetNumber NOTIFY numberChanged)
    Q_PROPERTY(bool shadow READ hasShadow WRITE setShadow RESET resetShadow NOTIFY shadowChanged)

public:
    enum Option { NoOptions = 0b0000, Stroke = 0b0001, Fill = 0b0010, Font = 0b0100, Shadow = 0b1000 };
    Q_DECLARE_FLAGS(Options, Option)
    Q_FLAG(Options)

    AnnotationTool(AnnotationDocument *document);
    ~AnnotationTool();

    AnnotationDocument::EditActionType type() const;
    void setType(AnnotationDocument::EditActionType type);
    void resetType();

    Options options() const;

    int strokeWidth() const;
    void setStrokeWidth(int width);
    void resetStrokeWidth();

    QColor strokeColor() const;
    void setStrokeColor(const QColor &color);
    void resetStrokeColor();

    QColor fillColor() const;
    void setFillColor(const QColor &color);
    void resetFillColor();

    QFont font() const;
    void setFont(const QFont &font);
    void resetFont();

    QColor fontColor() const;
    void setFontColor(const QColor &color);
    void resetFontColor();

    int number() const;
    void setNumber(int number);
    void resetNumber();

    bool hasShadow() const;
    void setShadow(bool shadow);
    void resetShadow();

Q_SIGNALS:
    void typeChanged();
    void optionsChanged();
    void strokeWidthChanged(int width);
    void strokeColorChanged(const QColor &color);
    void fillColorChanged(const QColor &color);
    void fontChanged(const QFont &font);
    void fontColorChanged(const QColor &color);
    void numberChanged(const int number);
    void shadowChanged(bool hasShadow);

private:
    int strokeWidthForType(AnnotationDocument::EditActionType type) const;
    void setStrokeWidthForType(int width, AnnotationDocument::EditActionType type);

    QColor strokeColorForType(AnnotationDocument::EditActionType type) const;
    void setStrokeColorForType(const QColor &color, AnnotationDocument::EditActionType type);

    QColor fillColorForType(AnnotationDocument::EditActionType type) const;
    void setFillColorForType(const QColor &color, AnnotationDocument::EditActionType type);

    QFont fontForType(AnnotationDocument::EditActionType type) const;
    void setFontForType(const QFont &font, AnnotationDocument::EditActionType type);

    QColor fontColorForType(AnnotationDocument::EditActionType type) const;
    void setFontColorForType(const QColor &color, AnnotationDocument::EditActionType type);

    bool typeHasShadow(AnnotationDocument::EditActionType type) const;
    void setTypeHasShadow(AnnotationDocument::EditActionType type, bool shadow);

    AnnotationDocument::EditActionType m_type = AnnotationDocument::None;
    Options m_options = Option::NoOptions;
    int m_number = 1;
};

/**
 * When the user selects an existing shape with the mouse, this wraps all the parameters of the associated action, so that they can be modified from QML
 */
class SelectedActionWrapper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AnnotationDocument::EditActionType type READ type CONSTANT)
    Q_PROPERTY(AnnotationTool::Options options READ options CONSTANT)
    Q_PROPERTY(int strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY strokeWidthChanged)
    Q_PROPERTY(QColor strokeColor READ strokeColor WRITE setStrokeColor NOTIFY strokeColorChanged)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(QColor fontColor READ fontColor WRITE setFontColor NOTIFY fontColorChanged)
    Q_PROPERTY(int number READ number WRITE setNumber NOTIFY numberChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QRectF visualGeometry READ visualGeometry WRITE setVisualGeometry NOTIFY visualGeometryChanged)
    Q_PROPERTY(bool shadow READ hasShadow WRITE setShadow NOTIFY shadowChanged)

public:
    SelectedActionWrapper(AnnotationDocument *document);
    ~SelectedActionWrapper();

    EditAction *editAction() const;
    void setEditAction(EditAction *action);

    bool isValid() const;

    AnnotationDocument::EditActionType type() const;

    AnnotationTool::Options options() const;

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

    QString text() const;
    void setText(const QString &text);

    QRectF visualGeometry() const;
    void setVisualGeometry(const QRectF &geom);

    bool hasShadow() const;
    void setShadow(bool shadow);

    Q_INVOKABLE void commitChanges();

Q_SIGNALS:
    void strokeWidthChanged();
    void strokeColorChanged();
    void fillColorChanged();
    void fontChanged();
    void fontColorChanged();
    void numberChanged();
    void textChanged();
    void visualGeometryChanged();
    void shadowChanged();

private:
    AnnotationDocument::EditActionType m_type = AnnotationDocument::None;
    AnnotationTool::Options m_options;
    EditAction *m_editAction = nullptr;
    std::unique_ptr<EditAction> m_actionCopy;
    AnnotationDocument *const m_document;
};

QDebug operator<<(QDebug debug, const SelectedActionWrapper *saw);

Q_DECLARE_OPERATORS_FOR_FLAGS(AnnotationDocument::ContinueOptions)
Q_DECLARE_OPERATORS_FOR_FLAGS(AnnotationDocument::RenderOptions)
Q_DECLARE_OPERATORS_FOR_FLAGS(AnnotationTool::Options)
