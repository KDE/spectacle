/*
 *  Copyright (C) 2016 Boudhayan Gupta <bgupta@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef QUICKEDITOR_H
#define QUICKEDITOR_H

#include <QObject>
#include <QVariant>

class QuickEditor : public QObject
{
    Q_OBJECT

    public:

    explicit QuickEditor(const QPixmap &pixmap, QObject *parent = 0);
    virtual ~QuickEditor();

    signals:

    void grabDone(const QPixmap &pixmap, const QRect &cropRegion);
    void grabCancelled();

    private slots:

    void acceptImageHandler(int x, int y, int width, int height,
                            QVariant canvasdata, int canvaswidth, int canvasheight);

    private:

    static QImage imageFromCanvasData(QList<QVariant> data, int width, int height);

    struct ImageStore;
    ImageStore *mImageStore;

    struct QuickEditorPrivate;
    Q_DECLARE_PRIVATE(QuickEditor)
    QuickEditorPrivate *d_ptr;
};

#endif // QUICKEDITOR_H
