/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <QQmlEngine>

#include <memory>

class QPainterPath;
class QPointF;
class QRectF;
class QSizeF;

/**
 * This is a class for processing geometry.
 *
 * This class has functionality for converting between logical screen coordinates
 * and platform native screen coordinates. For Wayland, there is no difference.
 *
 * On X11, QScreen::geometry has the raw size (in hardware pixels) scaled down by
 * the device pixel ratio (DPR), but the position is still the screen's raw position.
 *
 * On Wayland, QScreen::geometry has the raw size scaled down by the DPR and
 * the position is also adjusted so that each QScreen::geometry matches the
 * layout used by the Wayland compositor (almost, see the next paragraph).
 * This kind of geometry is sometimes called "logical" geometry.
 *
 * QScreen::geometry is a QRect, which means some precision is lost when the
 * raw size is scale down. If QScreen::geometry is scaled back up, it may not
 * match the raw size, especially when fractional scale factors are used or the
 * resolution has an odd width/height.
 */
class Geometry : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit Geometry(QObject *parent = nullptr);

    /**
     * This returns the logical size of a raw device pixel based on the given device pixel ratio.
     * This has a short name so that it doesn't make lines of code really long.
     */
    [[nodiscard]] Q_INVOKABLE static qreal dpx(qreal dpr);

    /**
     * This rounds a logical axis value to a value that should be aligned to hardware pixels
     * if the given device pixel ratio is correct.
     */
    [[nodiscard]] Q_INVOKABLE static qreal dprRound(qreal value, qreal dpr);

    /**
     * This rounds a point to a point that should be aligned to hardware pixels
     * if the given device pixel ratio is correct.
     */
    [[nodiscard]] Q_INVOKABLE static QPointF dprRound(QPointF point, qreal dpr);

    /**
     * This ceils a logical axis value to a value that should be aligned to hardware pixels
     * if the given device pixel ratio is correct.
     */
    [[nodiscard]] Q_INVOKABLE static qreal dprCeil(qreal value, qreal dpr);

    /**
     * This floors a logical axis value to a value that should be aligned to hardware pixels
     * if the given device pixel ratio is correct.
     */
    [[nodiscard]] Q_INVOKABLE static qreal dprFloor(qreal value, qreal dpr);

    /**
     * This converts an X11 screen axis value to a logical screen axis value.
     * Otherwise, it returns the value as it already was.
     */
    [[nodiscard]] Q_INVOKABLE static qreal mapFromPlatformValue(qreal value, qreal dpr);

    /**
     * This converts an X11 screen point to a logical screen point.
     * Otherwise, it returns the point as it already was.
     */
    [[nodiscard]] Q_INVOKABLE static QPointF mapFromPlatformPoint(const QPointF &point, qreal dpr);

    /**
     * This converts an X11 screen rect to a logical screen rect.
     * Otherwise, it returns the rect as it already was.
     */
    [[nodiscard]] Q_INVOKABLE static QRectF mapFromPlatformRect(const QRectF &rect, qreal dpr);

    /**
     * This returns the union of all logical screen rects.
     *
     * NOTE: Not perfectly accurate with some device pixel ratios
     * and resolutions due to QScreen::geometry being a QRect.
     */
    [[nodiscard]] Q_INVOKABLE static QRectF logicalScreensRect();

    /**
     * This converts a logical screen axis value to a platform screen axis value.
     * On Wayland, it returns the value as it already was.
     */
    [[nodiscard]] Q_INVOKABLE static qreal mapToPlatformValue(qreal value, qreal dpr);

    /**
     * This converts a logical screen point to a platform screen point.
     * On Wayland, it returns the point as it already was.
     */
    [[nodiscard]] Q_INVOKABLE static QPointF mapToPlatformPoint(const QPointF &point, qreal dpr);

    /**
     * This converts a logical screen rect to a platform screen rect.
     * On Wayland, it returns the rect as it already was.
     */
    [[nodiscard]] Q_INVOKABLE static QRectF mapToPlatformRect(const QRectF &rect, qreal dpr);

    /**
     * This returns the union of all platform screen rects.
     *
     * NOTE: Not perfectly accurate with some device pixel ratios
     * and resolutions due to QScreen::geometry being a QRect.
     */
    [[nodiscard]] Q_INVOKABLE static QRectF platformUnifiedRect();

    /**
     * This returns a raw size for a logical size by multiplying by the DPR.
     * NOTE: The value will only be correct if no precision was lost prior to running the function.
     */
    [[nodiscard]] Q_INVOKABLE static QSize rawSize(const QSizeF &size, qreal dpr);

    [[nodiscard]] Q_INVOKABLE static QRectF rectNormalized(const QRectF &rect);
    [[nodiscard]] Q_INVOKABLE static QRectF rectNormalized(qreal x, qreal y, qreal w, qreal h);

    /// Get the rectangle with adjusted left, top, right and bottom sides.
    [[nodiscard]] Q_INVOKABLE static QRectF rectAdjusted(const QRectF &rect, qreal xp1, qreal yp1, qreal xp2, qreal yp2);

    /**
     * Get the rectangle with adjusted left, top, right and bottom sides.
     * This affects the sides of invalid rectangles as if they were normalized.
     * If the rect is invalid, that will be preserved.
     */
    [[nodiscard]] Q_INVOKABLE static QRectF rectAdjustedVisually(const QRectF &rect, qreal xp1, qreal yp1, qreal xp2, qreal yp2);

    /// Get a rect with a size and position multiplied by the scale.
    [[nodiscard]] Q_INVOKABLE static QRectF rectScaled(const QRectF &rect, qreal scale);

    /// Get the intersection of two rectangles.
    [[nodiscard]] Q_INVOKABLE static QRectF rectIntersected(const QRectF &rect1, const QRectF &rect2);

    /**
     * Try to make the rect positioned fully inside boundsRect without clipping on the given axes.
     * The rect may still be out of bounds if the size is too large.
     * If the rect is invalid, that will be preserved.
     */
    [[nodiscard]] Q_INVOKABLE static QRectF rectBounded(const QRectF &rect, const QRectF &boundsRect, //
                                                        Qt::Orientations orientations = Qt::Horizontal | Qt::Vertical);
    [[nodiscard]] Q_INVOKABLE static QRectF rectBounded(qreal x, qreal y, qreal width, qreal height, const QRectF &boundsRect, //
                                                        Qt::Orientations orientations = Qt::Horizontal | Qt::Vertical);

    /**
     * Try to make the rect positioned fully inside the boundsPath without clipping.
     * The rect may still be out of bounds if the size is too large.
     * If the rect is invalid, that will be preserved.
     */
    [[nodiscard]] Q_INVOKABLE static QRectF rectBounded(const QRect &rect, const QPainterPath &boundsPath);

    /**
     * Clip the rect to the clipRect on the given axes.
     * If the rect does not intersect with the clipRect,
     * it will become an empty rect on the edge of clipRect nearest to the original position.
     * If the rect is invalid, that will be preserved.
     */
    [[nodiscard]] Q_INVOKABLE static QRectF rectClipped(const QRectF &rect, const QRectF &clipRect, //
                                                        Qt::Orientations orientations = Qt::Horizontal | Qt::Vertical);

    /// Check if the rectangle contains the value on the given axes.
    [[nodiscard]] Q_INVOKABLE static bool rectContains(const QRectF &rect, qreal v, //
                                                       Qt::Orientations orientations = Qt::Horizontal | Qt::Vertical);
    /// Check if the rectangle contains the point.
    [[nodiscard]] Q_INVOKABLE static bool rectContains(const QRectF &rect, qreal x, qreal y);
    [[nodiscard]] Q_INVOKABLE static bool rectContains(const QRectF &rect, const QPointF& point);
    /// Check if the first rectangle contains the second rectangle.
    [[nodiscard]] Q_INVOKABLE static bool rectContains(const QRectF &rect1, const QRectF& rect2);
    [[nodiscard]] Q_INVOKABLE static bool rectContains(const QRectF &rect, qreal x, qreal y, qreal w, qreal h);

    /// Check if the ellipse contains the point.
    [[nodiscard]] Q_INVOKABLE static bool ellipseContains(qreal ellipseX, qreal ellipseY,
                                                          qreal ellipseWidth, qreal ellipseHeight,
                                                          qreal x, qreal y);
    /// Assuming the rectangle represents an ellipse's bounding box, check if it contains the point.
    [[nodiscard]] Q_INVOKABLE static bool ellipseContains(const QRectF &rect, qreal x, qreal y);
    [[nodiscard]] Q_INVOKABLE static bool ellipseContains(const QRectF &rect, const QPointF &point);

    /// Check if two rectangles intersect.
    [[nodiscard]] Q_INVOKABLE static bool rectIntersects(const QRectF &rect1, const QRectF& rect2);
    [[nodiscard]] Q_INVOKABLE static bool rectIntersects(const QRectF &rect, qreal x, qreal y, qreal w, qreal h);

    /// Is width or height less than or equal to 0
    [[nodiscard]] Q_INVOKABLE static bool isEmpty(qreal w, qreal h);
    [[nodiscard]] Q_INVOKABLE static bool isEmpty(const QSizeF &size);
    [[nodiscard]] Q_INVOKABLE static bool isEmpty(const QRectF &rect);

    /// Are width and height equal to 0
    [[nodiscard]] Q_INVOKABLE static bool isNull(qreal w, qreal h);
    [[nodiscard]] Q_INVOKABLE static bool isNull(const QSizeF &size);
    [[nodiscard]] Q_INVOKABLE static bool isNull(const QRectF &rect);

private:
    Geometry(const Geometry &) = delete;
    Geometry(Geometry &&) = delete;
    Geometry &operator=(const Geometry &) = delete;
    Geometry &operator=(Geometry &&) = delete;
    friend class GeometrySingleton;
};
