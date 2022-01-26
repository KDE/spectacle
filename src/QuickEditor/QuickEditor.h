/*
 *  SPDX-FileCopyrightText: 2018 Ambareesh "Amby" Balaji <ambareeshbalaji@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef QUICKEDITOR_H
#define QUICKEDITOR_H

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QPointer>

namespace KWayland::Client
{
class PlasmaShell;
class PlasmaShellSurface;
}

class AreaSelectorItem;

class QuickEditorView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit QuickEditorView(QGraphicsScene *scene, KWayland::Client::PlasmaShell *plasmashell, QScreen *screen);
    ~QuickEditorView() override;

private:
    void relayout();

    QPointer<QScreen> mDesiredScreen;
    QScopedPointer<KWayland::Client::PlasmaShellSurface> mPlasmaShellSurface;
};

class QuickEditor : public QObject
{
    Q_OBJECT

public:
    QuickEditor(const QMap<const QScreen *, QImage> &images, KWayland::Client::PlasmaShell *plasmashell, QObject *parent = nullptr);
    ~QuickEditor() override;

Q_SIGNALS:
    void grabDone(const QPixmap &thePixmap);
    void grabCancelled();

private:
    QImage captureSelectionX11(const QRect &selection);
    QImage captureSelectionWayland(const QRect &selection);
    void captureSelection();

    QMap<const QScreen *, QImage> mImages;
    QGraphicsScene *mScene;
    QList<QuickEditorView *> mViews;
    AreaSelectorItem *mSelectorItem;
};

#endif // QUICKEDITOR_H
