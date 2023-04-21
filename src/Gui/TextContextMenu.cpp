/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "TextContextMenu.h"

#include <QPlainTextEdit>
#include <QQuickItem>

class TextContextMenuSingleton
{
public:
    TextContextMenu self;
};

Q_GLOBAL_STATIC(TextContextMenuSingleton, privateTextContextMenuSelf)

TextContextMenu::TextContextMenu(QWidget *parent)
    : SpectacleMenu(parent)
{
    std::unique_ptr<QPlainTextEdit> plainTextEdit(new QPlainTextEdit);
    auto contextMenu = plainTextEdit->createStandardContextMenu();
    const auto actions = contextMenu->actions();
    for (auto action : actions) {
        const auto objectName = action->objectName().toLatin1();
        if (action->isSeparator()) {
            addAction(action);
            action->setParent(this);
        } else if (!objectName.isEmpty()) {
            disconnect(action, &QAction::triggered, nullptr, nullptr);
            if (!undoAction && objectName == "edit-undo") {
                undoAction = action;
            } else if (!redoAction && objectName == "edit-redo") {
                redoAction = action;
            } else if (!cutAction && objectName == "edit-cut") {
                cutAction = action;
            } else if (!copyAction && objectName == "edit-copy") {
                copyAction = action;
            } else if (objectName == "link-copy") {
                continue;
            } else if (!pasteAction && objectName == "edit-paste") {
                pasteAction = action;
            } else if (!deleteAction && objectName == "edit-delete") {
                deleteAction = action;
            } else if (!selectAllAction && objectName == "select-all") {
                selectAllAction = action;
            }
            addAction(action);
            action->setParent(this);
        }
    }
}

TextContextMenu *TextContextMenu::instance()
{
    return &privateTextContextMenuSelf->self;
}

void TextContextMenu::popup(QQuickItem *editor)
{
    if (!editor || !(editor->inherits("QQuickTextEdit") || editor->inherits("QQuickTextInput"))) {
        return;
    }

    const bool readOnly = editor->property("readOnly").toBool();
    const bool interactiveSelection = editor->property("selectByMouse").toBool()
                                   || editor->property("selectByKeyboard").toBool();
    const bool hasSelection = !editor->property("selectedText").toString().isEmpty();

    undoAction->setVisible(!readOnly);
    undoAction->setEnabled(editor->property("canUndo").toBool());

    redoAction->setVisible(!readOnly);
    redoAction->setEnabled(editor->property("canRedo").toBool());

    cutAction->setVisible(!readOnly);
    cutAction->setEnabled(hasSelection);

    copyAction->setVisible(interactiveSelection);
    copyAction->setEnabled(hasSelection);

    pasteAction->setVisible(!readOnly);
    pasteAction->setEnabled(editor->property("canPaste").toBool());

    deleteAction->setVisible(!readOnly);
    deleteAction->setEnabled(hasSelection);

    selectAllAction->setVisible(interactiveSelection);
    selectAllAction->setEnabled(editor->property("length").toInt() > 0);

    const auto actions = this->actions();
    bool hasVisibleActions = false;
    for (int i = 0; i < actions.size(); ++i) {
        disconnect(actions[i], &QAction::triggered, nullptr, nullptr);
        if (i < actions.size() - 1 && actions[i]->isSeparator()) {
            actions[i]->setVisible(actions[i + 1]->isVisible());
        }
        hasVisibleActions |= actions[i]->isVisible();
    }

    if (!hasVisibleActions) {
        return;
    }

    connect(undoAction, &QAction::triggered, editor, [editor](){
        QMetaObject::invokeMethod(editor, "undo");
    });
    connect(redoAction, &QAction::triggered, editor, [editor](){
        QMetaObject::invokeMethod(editor, "redo");
    });
    connect(cutAction, &QAction::triggered, editor, [editor](){
        QMetaObject::invokeMethod(editor, "cut");
    });
    connect(copyAction, &QAction::triggered, editor, [editor](){
        QMetaObject::invokeMethod(editor, "copy");
    });
    connect(pasteAction, &QAction::triggered, editor, [editor](){
        QMetaObject::invokeMethod(editor, "paste");
    });
    connect(deleteAction, &QAction::triggered, editor, [editor](){
        QMetaObject::invokeMethod(
            editor, "remove",
            Q_ARG(int, editor->property("selectionStart").toInt()),
            Q_ARG(int, editor->property("selectionEnd").toInt())
        );
    });
    connect(selectAllAction, &QAction::triggered, editor, [editor](){
        QMetaObject::invokeMethod(editor, "selectAll");
    });

    // Don't make the menu appear far away
    if (editor->width() > width() || editor->height() >= fontMetrics().height() * 2) {
        QMenu::popup(QCursor::pos());
        return;
    }
    const bool useParentPosition = editor->parentItem() && editor->parentItem()->inherits("QQuickSpinBox");
    SpectacleMenu::popup(useParentPosition ? editor->parentItem() : editor);
}
