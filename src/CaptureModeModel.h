/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "Platforms/Platform.h"

#include <QAbstractListModel>

/**
 * This is a model containing the current supported capture modes and their labels and shortcuts.
 */
class CaptureModeModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)
public:
    CaptureModeModel(Platform::GrabModes grabModes, QObject *parent = nullptr);

    enum CaptureMode {
        RectangularRegion,
        AllScreens,
        // TODO: find a more user configuration friendly way to scale source images
        AllScreensScaled,
        CurrentScreen,
        ActiveWindow,
        WindowUnderCursor
    };
    Q_ENUM(CaptureMode)

    enum {
        CaptureModeRole = Qt::UserRole + 1,
        ShortcutsRole = Qt::UserRole + 2,
    };

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    int indexOfCaptureMode(CaptureMode captureMode) const;

    void setGrabModes(Platform::GrabModes modes);

Q_SIGNALS:
    void captureModesChanged();
    void countChanged();

private:
    struct Item {
        CaptureModeModel::CaptureMode captureMode;
        QString label;
        QString shortcuts; // default value in case there's nothing
    };

    QVector<Item> m_data;
    QHash<int, QByteArray> m_roleNames;
    Platform::GrabModes m_grabModes;
};
