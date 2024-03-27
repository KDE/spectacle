/* SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QColor>
#include <QFont>
#include <QObject>

/**
 * This is the data structure that controls the creation of the next item. From qml its paramenter
 * will be set by the app toolbars, and then drawing on the screen with the mouse will lead to the
 * creation of a new item based on those parameters
 */
class AnnotationTool : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Tool type READ type WRITE setType RESET resetType NOTIFY typeChanged)
    Q_PROPERTY(bool isNoTool READ isNoTool NOTIFY typeChanged)
    Q_PROPERTY(bool isMetaTool READ isMetaTool NOTIFY typeChanged)
    Q_PROPERTY(bool isCreationTool READ isCreationTool NOTIFY typeChanged)
    Q_PROPERTY(Options options READ options NOTIFY optionsChanged)
    Q_PROPERTY(int strokeWidth READ strokeWidth WRITE setStrokeWidth RESET resetStrokeWidth NOTIFY strokeWidthChanged)
    Q_PROPERTY(QColor strokeColor READ strokeColor WRITE setStrokeColor RESET resetStrokeColor NOTIFY strokeColorChanged)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor RESET resetFillColor NOTIFY fillColorChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont RESET resetFont NOTIFY fontChanged)
    Q_PROPERTY(QColor fontColor READ fontColor WRITE setFontColor RESET resetFontColor NOTIFY fontColorChanged)
    Q_PROPERTY(int number READ number WRITE setNumber RESET resetNumber NOTIFY numberChanged)
    Q_PROPERTY(bool shadow READ hasShadow WRITE setShadow RESET resetShadow NOTIFY shadowChanged)

public:
    /**
     * These tools are meant to be shown as selectable tool types in the UI.
     * They can also affect the types of traits a drawable object is allowed to have.
     */
    enum Tool {
        NoTool,
        // Meta tools
        SelectTool,
        // Creation tools
        FreehandTool,
        HighlighterTool,
        LineTool,
        ArrowTool,
        RectangleTool,
        EllipseTool,
        BlurTool,
        PixelateTool,
        TextTool,
        NumberTool,
        NTools,
    };
    Q_ENUM(Tool)

    /**
     * These options are meant to help control which options are visible in the UI
     * and what kinds of traits a drawable object should have.
     */
    enum Option {
        NoOptions = 0,
        StrokeOption = 1,
        FillOption = 1 << 1,
        FontOption = 1 << 2,
        TextOption = 1 << 3,
        NumberOption = 1 << 4,
        ShadowOption = 1 << 5,
    };
    Q_DECLARE_FLAGS(Options, Option)
    Q_FLAG(Options)

    AnnotationTool(QObject *parent);
    ~AnnotationTool();

    Tool type() const;
    void setType(Tool type);
    void resetType();

    bool isNoTool() const;

    // Whether the current tool type is for modifying the document's attributes.
    bool isMetaTool() const;

    // Whether the current tool type is for creating annotation objects.
    bool isCreationTool() const;

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
    static constexpr AnnotationTool::Options optionsForType(AnnotationTool::Tool type);

    static constexpr int defaultStrokeWidthForType(AnnotationTool::Tool type);
    int strokeWidthForType(Tool type) const;
    void setStrokeWidthForType(int width, Tool type);

    static constexpr QColor defaultStrokeColorForType(AnnotationTool::Tool type);
    QColor strokeColorForType(Tool type) const;
    void setStrokeColorForType(const QColor &color, Tool type);

    static constexpr QColor defaultFillColorForType(AnnotationTool::Tool type);
    QColor fillColorForType(Tool type) const;
    void setFillColorForType(const QColor &color, Tool type);

    QFont fontForType(Tool type) const;
    void setFontForType(const QFont &font, Tool type);

    static constexpr QColor defaultFontColorForType(AnnotationTool::Tool type);
    QColor fontColorForType(Tool type) const;
    void setFontColorForType(const QColor &color, Tool type);

    bool typeHasShadow(Tool type) const;
    void setTypeHasShadow(Tool type, bool shadow);

    Tool m_type = Tool::NoTool;
    Options m_options = Option::NoOptions;
    int m_number = 1;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AnnotationTool::Options)
