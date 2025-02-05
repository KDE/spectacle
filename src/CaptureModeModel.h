/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "Platforms/ImagePlatform.h"

#include <QAbstractListModel>
#include <QQmlEngine>

/**
 * This is a model containing the current supported capture modes and their labels and shortcuts.
 */
class CaptureModeModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

public:
    CaptureModeModel(QObject *parent = nullptr);

    static CaptureModeModel *instance();

    static CaptureModeModel *create(QQmlEngine *engine, QJSEngine *)
    {
        auto inst = instance();
        Q_ASSERT(inst);
        Q_ASSERT(inst->thread() == engine->thread());
        QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
        return inst;
    }

    enum CaptureMode {
        RectangularRegion,
        AllScreens,
        // TODO: find a more user configuration friendly way to scale source images
        AllScreensScaled,
        CurrentScreen,
        ActiveWindow,
        WindowUnderCursor,
        FullScreen,
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

    void setGrabModes(ImagePlatform::GrabModes modes);

    static QString captureModeLabel(CaptureMode mode);

Q_SIGNALS:
    void captureModesChanged();
    void countChanged();

private:
    struct Item {
        CaptureModeModel::CaptureMode captureMode;
        QString label;
        QString shortcuts = {}; // default value in case there's nothing
    };

    QList<Item> m_data;
    QHash<int, QByteArray> m_roleNames;
    ImagePlatform::GrabModes m_grabModes;
};
