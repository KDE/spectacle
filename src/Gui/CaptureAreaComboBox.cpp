/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@zohomail.eu>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "CaptureAreaComboBox.h"

#include "ShortcutActions.h"

#include <KColorScheme>
#include <KGlobalAccel>
#include <KLocalizedString>

#include <QAbstractItemView>
#include <QApplication>
#include <QKeySequence>
#include <QPainter>
#include <QString>
#include <QStyledItemDelegate>

#include <unordered_map>

/**
 * @brief Custom Delegate for the ComboBox popup with support for displaying shortcuts.
 */
class CaptureModeDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    /**
     * Maps CaptureModes to their keyboard shortcuts as QStrings.
     */
    std::unordered_map<Spectacle::CaptureMode, QString> m_shortcutTextsMap;

    /**
     * Populates the m_shortcutTextsMap with the current shortcuts for the various Spectacle::CaptureModes.
     *
     * This method won't be able to determine the global shortcuts if SpectacleCore::setUpShortcuts() hasn't been called yet.
     * @see SpectacleCore::setUpShortcuts()
     */
    void updateShortcutTexts()
    {
        KGlobalAccel *accelerator = KGlobalAccel::self();

        auto shortcutTextForAction = [accelerator](QAction *action) {
            if (accelerator->hasShortcut(action) && !accelerator->shortcut(action).isEmpty()) {
                return accelerator->shortcut(action).constFirst().toString(QKeySequence::NativeText);
            }
            return QString{};
        };

        auto actions = ShortcutActions::self(); // retrieve the capture actions
        m_shortcutTextsMap[Spectacle::CaptureMode::AllScreens] = shortcutTextForAction(actions->fullScreenAction());
        m_shortcutTextsMap[Spectacle::CaptureMode::CurrentScreen] = shortcutTextForAction(actions->currentScreenAction());
        m_shortcutTextsMap[Spectacle::CaptureMode::ActiveWindow] = shortcutTextForAction(actions->activeWindowAction());
        m_shortcutTextsMap[Spectacle::CaptureMode::WindowUnderCursor] = shortcutTextForAction(actions->windowUnderCursorAction());
        m_shortcutTextsMap[Spectacle::CaptureMode::RectangularRegion] = shortcutTextForAction(actions->regionAction());
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        const QWidget *widget = option.widget;
        const QStyle *style = widget ? widget->style() : QApplication::style();

        // The Breeze style prevents itself from styling this itemDelegate because it expects to be incompatible with such custom QStyledItemDelegates.
        // We manually style it to look like Breeze because we know that this will be a compatible change.
        bool breezeStyle = false;
        const QMetaObject *styleMetaObject = style->metaObject();
        while (!breezeStyle && styleMetaObject) {
            if (QString::fromUtf8(styleMetaObject->className()) == QStringLiteral("Breeze::Style")) {
                breezeStyle = true;
                break;
            }
            styleMetaObject = styleMetaObject->superClass();
        }
        if (breezeStyle) {
            // We paint the selection area a certain way.
            if (option.showDecorationSelected && (option.state & QStyle::State_Selected)) {
                auto color = option.palette.brush((option.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled, QPalette::Highlight).color();

                painter->setPen(color);
                color.setAlphaF(color.alphaF() * 0.3);
                painter->setBrush(color);
                constexpr int radius = 2;
                painter->drawRoundedRect(QRectF(option.rect).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
            }
            // We paint the text the same way no matter what. (We ignore if the entry is selected or hovered.)
            auto optionCopy = option;
            optionCopy.showDecorationSelected = false;
            optionCopy.state &= ~QStyle::State_Selected;
            optionCopy.state &= ~QStyle::State_MouseOver;
            QStyledItemDelegate::paint(painter, optionCopy, index);
        } else {
            QStyledItemDelegate::paint(painter, option, index);
        }

        auto captureMode = static_cast<Spectacle::CaptureMode>(index.data(Qt::UserRole).toInt());

        // Draw the shortcutText
        if (m_shortcutTextsMap.count(captureMode) == 0) {
            return; // There is no shortcutText for this captureMode.
        }

        if (option.state & QStyle::State_Selected && !breezeStyle) {
            painter->setPen(option.palette.color(QPalette::HighlightedText));
        } else {
            const KColorScheme colorScheme = KColorScheme(QPalette::Normal, KColorScheme::View);
            const QColor inactiveTextColor = colorScheme.foreground(KColorScheme::InactiveText).color();
            painter->setPen(inactiveTextColor);
        }

        QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &option);
        int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &option, widget) + 1;
        textRect.adjust(textMargin, 0, -textMargin, 0);
        painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, m_shortcutTextsMap.at(captureMode));
    }
};

CaptureAreaComboBox::CaptureAreaComboBox(Platform::GrabModes grabModes, QWidget *parent)
    : QComboBox{parent}
    , mGrabModes{grabModes}
{
    // Initialise the model
    if (grabModes.testFlag(Platform::GrabMode::AllScreens)) {
        QString lFullScreenLabel = QApplication::screens().count() == 1 ? i18n("Full Screen") : i18n("Full Screen (All Monitors)");

        insertItem(0, lFullScreenLabel, Spectacle::CaptureMode::AllScreens);
    }
    if (grabModes.testFlag(Platform::GrabMode::AllScreensScaled) && QApplication::screens().count() > 1) {
        QString lFullScreenLabel = i18n("Full Screen (All Monitors, scaled)");
        insertItem(1, lFullScreenLabel, Spectacle::CaptureMode::AllScreensScaled);
    }
    if (grabModes.testFlag(Platform::GrabMode::PerScreenImageNative)) {
        insertItem(2, i18n("Rectangular Region"), Spectacle::CaptureMode::RectangularRegion);
    }
    if (grabModes.testFlag(Platform::GrabMode::CurrentScreen)) {
        insertItem(3, i18n("Current Screen"), Spectacle::CaptureMode::CurrentScreen);
    }
    if (grabModes.testFlag(Platform::GrabMode::ActiveWindow)) {
        insertItem(4, i18n("Active Window"), Spectacle::CaptureMode::ActiveWindow);
    }
    if (grabModes.testFlag(Platform::GrabMode::WindowUnderCursor)) {
        insertItem(5, i18n("Window Under Cursor"), Spectacle::CaptureMode::WindowUnderCursor);
    }

    // Use our custom delegate with support for displaying shortcuts in the popup.
    mCaptureModeDelegate = new CaptureModeDelegate{this};
    setItemDelegate(mCaptureModeDelegate);
}

void CaptureAreaComboBox::showPopup()
{
    mCaptureModeDelegate->updateShortcutTexts();

    int widestTextWidth = 0;
    for (int i = 0; i < count(); ++i) {
        const int leftWidth = fontMetrics().horizontalAdvance(itemText(i));

        auto captureMode = captureModeForIndex(i);
        if (mCaptureModeDelegate->m_shortcutTextsMap.count(captureMode) == 0) {
            continue; // There is no shortcutText for this captureMode.
        }
        const int rightWidth = fontMetrics().horizontalAdvance(mCaptureModeDelegate->m_shortcutTextsMap.at(captureMode));

        widestTextWidth = std::max(widestTextWidth, leftWidth + rightWidth);
    }
    constexpr int minimumSpacingBetweenLeftAndRightText = 10;
    if (width() < widestTextWidth + minimumSpacingBetweenLeftAndRightText) { // We widen the popup if the left and right text would overlap.
        view()->setMinimumWidth(widestTextWidth + minimumSpacingBetweenLeftAndRightText * 2);
    }

    QComboBox::showPopup();
}
