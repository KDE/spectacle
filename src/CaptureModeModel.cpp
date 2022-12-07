/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "CaptureModeModel.h"
#include "ShortcutActions.h"

#include <KGlobalAccel>
#include <KLocalizedString>

#include <QApplication>
#include <qnamespace.h>
#include <utility>

static QString actionShortcutsToString(QAction *action)
{
    QString value;
    if (!KGlobalAccel::self()->hasShortcut(action)) {
        return value;
    }

    const auto &shortcuts = KGlobalAccel::self()->shortcut(action);
    for (int i = 0; i < shortcuts.length(); ++i) {
        if (i > 0) {
            value.append(QStringLiteral(", "));
        }
        value.append(shortcuts[i].toString(QKeySequence::NativeText));
    }
    return value;
}

CaptureModeModel::CaptureModeModel(Platform::GrabModes grabModes, QObject *parent)
    : QAbstractListModel(parent)
{
    m_roleNames[CaptureModeRole] = QByteArrayLiteral("captureMode");
    m_roleNames[Qt::DisplayRole] = QByteArrayLiteral("display");
    m_roleNames[ShortcutsRole] = QByteArrayLiteral("shortcuts");
    setGrabModes(grabModes);
}

QHash<int, QByteArray> CaptureModeModel::roleNames() const
{
    return m_roleNames;
}

QVariant CaptureModeModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    QVariant ret;
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return ret;
    }
    if (role == CaptureModeRole) {
        ret = m_data.at(row).captureMode;
    } else if (role == Qt::DisplayRole) {
        ret = m_data.at(row).label;
    } else if (role == ShortcutsRole) {
        ret = m_data.at(row).shortcuts;
    }
    return ret;
}

int CaptureModeModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_data.size();
}

int CaptureModeModel::indexOfCaptureMode(CaptureMode captureMode) const
{
    int finalIndex = -1;
    for (int i = 0; i < m_data.length(); ++i) {
        if (m_data[i].captureMode == captureMode) {
            finalIndex = i;
            break;
        }
    }
    return finalIndex;
}

void CaptureModeModel::setGrabModes(Platform::GrabModes modes)
{
    if(m_grabModes == modes) {
        return;
    }
    m_grabModes = modes;
    const int oldCount = m_data.size();
    m_data.clear();

    int screenCount = QApplication::screens().count();

    if (m_grabModes.testFlag(Platform::GrabMode::PerScreenImageNative)) {
        m_data.append({
            CaptureModeModel::RectangularRegion,
            i18n("Rectangular Region"),
            actionShortcutsToString(ShortcutActions::self()->regionAction())
        });
    }
    if (m_grabModes.testFlag(Platform::GrabMode::AllScreens)) {
        m_data.append({
            CaptureModeModel::AllScreens,
            screenCount > 1 ? i18n("All Screens") : i18n("Full Screen"),
            actionShortcutsToString(ShortcutActions::self()->fullScreenAction())
        });
    }
    if (m_grabModes.testFlag(Platform::GrabMode::AllScreensScaled)) {
        m_data.append({
            CaptureModeModel::AllScreensScaled,
            i18n("All Screens (Scaled to same size)"),
            QStringLiteral("")
        });
    }
    if (m_grabModes.testFlag(Platform::GrabMode::CurrentScreen)) {
        m_data.append({
            CaptureModeModel::CurrentScreen,
            i18n("Current Screen"),
            actionShortcutsToString(ShortcutActions::self()->currentScreenAction())
        });
    }
    if (m_grabModes.testFlag(Platform::GrabMode::ActiveWindow)) {
        m_data.append({
            CaptureModeModel::ActiveWindow,
            i18n("Active Window"),
            actionShortcutsToString(ShortcutActions::self()->activeWindowAction())
        });
    }
    if (m_grabModes.testFlag(Platform::GrabMode::WindowUnderCursor)) {
        m_data.append({
            CaptureModeModel::WindowUnderCursor,
            i18n("Window Under Cursor"),
            actionShortcutsToString(ShortcutActions::self()->windowUnderCursorAction())
        });
    }

    Q_EMIT captureModesChanged();
    if (oldCount != m_data.size()) {
        Q_EMIT countChanged();
    }
}
