/* SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "Traits.h"

class HistoryItem;
class History;

/* Basically the same as owner_equal from C++26.
 * Needed to compare weak_ptrs without calling lock() all the time.
 *
 * From https://en.cppreference.com/w/cpp/memory/owner_equal:
 *
 * This function object provides owner-based (as opposed to value-based) mixed-type equal comparison
 * of both std::weak_ptr and std::shared_ptr. The comparison is such that two smart pointers compare
 * equivalent only if they are both empty or if they share ownership, even if the values of the raw
 * pointers obtained by get() are different (e.g. because they point at different subobjects within
 * the same object).
 *
 * NOTE: This is not what you want if you're using shared_ptr's aliasing constructor and you want
 * to compare the actual pointed to addresses, but the aliasing constructor is rarely needed.
 */
template<typename T1, typename T2>
inline bool operator==(const std::weak_ptr<T1> &lhs, const std::weak_ptr<T2> &rhs) noexcept
{
    return !lhs.owner_before(rhs) && !rhs.owner_before(lhs);
}
template<typename T1, typename T2>
inline bool operator==(const std::weak_ptr<T1> &lhs, const std::shared_ptr<T2> &rhs) noexcept
{
    return !lhs.owner_before(rhs) && !rhs.owner_before(lhs);
}
template<typename T1, typename T2>
inline bool operator==(const std::shared_ptr<T1> &lhs, const std::weak_ptr<T2> &rhs) noexcept
{
    return !lhs.owner_before(rhs) && !rhs.owner_before(lhs);
}

/**
 * A class that represents a state change in the undo/redo history.
 *
 * This is meant to be mostly externally immutable when managed in history.
 * This way we're not trying to keep track of changes happening in a million different places.
 * With this trait based structure, we're describing how to render an item in a generic way instead
 * of giving every combination a distinct class.
 *
 * We aren't using Qt's Undo Framework because it doesn't fit our needs. We want to use shared
 * pointers, we'd have to make major changes to the way AnnotationDocument works to switch and we'd
 * need a lot of different classes for different types of commands.
 *
 * We're using a tuple for traits instead of a container because the standard APIs for dealing
 * with multiple types are a bit nicer. Otherwise, we'd need to use something like std::variant.
 * That isn't terrible, but it's an extra layer to deal with and requires more code to be written.
 * If the memory usage of a tuple turns out to be too wasteful, we can switch to an unordered set or
 * unordered map of variants.
 *
 * We're using std::optional for traits because it provides a null state we can use to say that a
 * trait isn't enabled. The memory that the underlying object uses is allocated, but the object is
 * not constructed until you set a value. It may be less memory efficient than a pointer, but
 * std::optional should be fast when setting values since the memory is already allocated.
 */
class HistoryItem
{
public:
    using unique_ptr = std::unique_ptr<HistoryItem>;
    using shared_ptr = std::shared_ptr<HistoryItem>;
    using const_shared_ptr = std::shared_ptr<const HistoryItem>;
    using weak_ptr = shared_ptr::weak_type;
    using const_weak_ptr = const_shared_ptr::weak_type;

    bool operator==(const HistoryItem &other) const = default;

    bool hasParent() const;
    HistoryItem::const_weak_ptr parent() const;

    bool hasChild() const;
    HistoryItem::const_weak_ptr child() const;

    // Get a const reference to the tuple of all traits.
    const Traits::OptTuple &traits() const;

    // Get a reference to the tuple of all traits.
    Traits::OptTuple &traits(); // can modify

    // Whether this item's traits and parent properties are valid.
    bool isValid() const;

    // Whether this item can be seen by a user. Does not account for child items.
    bool visibleTraits() const;

    // The area that this item renders over.
    // This uses the parent's renderRect() when this item is not visible.
    QRectF renderRect() const;

    // Set the parent as the child's parent and the child as the parent's child.
    // I tried inheriting std::enable_shared_from_this to create a more automatic solution with
    // constructors, but weak_from_this() always immediately expired and shared_from_this()
    // caused segfaults.
    static void setItemRelations(shared_ptr parent, shared_ptr child);
    static void setItemRelations(const_shared_ptr parent, const_shared_ptr child);
    static void setItemRelations(shared_ptr parent, const_shared_ptr child);
    static void setItemRelations(const_shared_ptr parent, shared_ptr child);

protected:
    friend History;
    friend QDebug operator<<(QDebug debug, const HistoryItem &item);
    friend QDebug operator<<(QDebug debug, const HistoryItem *item);
    // The parent and child are mutable because they need to be changed even when this is const.
    // Using optional as a way to detect when parent has been set previously.
    mutable std::optional<HistoryItem::const_weak_ptr> m_parent;
    mutable HistoryItem::const_weak_ptr m_child;
    Traits::OptTuple m_traits;
};

QDebug operator<<(QDebug debug, const HistoryItem &item);
QDebug operator<<(QDebug debug, const HistoryItem *item);

/**
 * A class for managing an undo list and a redo list.
 * It is possible to assign variables to different history objects and keep multiple of them.
 * Redo objects are stored in reverse chronological order. This is because QList/vector
 * has O(1) complexity when inserting/erasing at the end and maximum O(n) complexity when
 * inserting/erasing at the start.
 */
class History
{
public:
    using List = QList<HistoryItem::shared_ptr>;
    using ConstList = QList<HistoryItem::const_shared_ptr>;
    struct ListsChangedResult {
        bool undoListChanged = false;
        bool redoListChanged = false;
    };

    struct ItemReplacedResult {
        HistoryItem::shared_ptr item;
        bool redoListChanged = false;
    };

    struct Lists {
        History::List undoList;
        History::List redoList;
    };

    History() = default;

    // Construct from two history lists.
    History(const List &undoList, const List &redoList);

    bool operator==(const History &other) const;

    const ConstList &undoList() const;
    const ConstList &redoList() const;

    // The index of the last undo object or -1 if the list is empty.
    List::size_type currentIndex() const;

    // The object at the end of the undo list or null if not available.
    HistoryItem::shared_ptr currentItem() const;

    // Get filtered copies of the undo and redo lists using the given function.
    Lists filteredLists(std::function<bool(History::List::const_reference)> function) const;

    // Push a new object onto the end of the undo list and clear the redo list.
    // Returns whether the undo and redo lists changed.
    ListsChangedResult push(List::const_reference item);

    // Pop the last object on the undo list.
    // Returns the popped object if successful or a default constructed object if not
    // and also whether the redo list changed.
    ItemReplacedResult pop();

    // Move the last object of the undo list to the end of the redo list.
    // Returns true if successful.
    bool undo();

    // Move the last object of the redo list to the end of the undo list.
    // Returns true if successful.
    bool redo();

    // Clear the redo list.
    // Returns true if the size changed.
    bool clearRedoList();

    // Clear the undo and redo lists.
    // Returns whether or not the lists changed.
    ListsChangedResult clearLists();

    // Whether the item is visible, in the undo list and without a child also in the undo list.
    bool itemVisible(ConstList::const_reference item) const;

protected:
    friend QDebug operator<<(QDebug debug, const History &history);
    // These are not public because we need to manage the child and parent traits of each item.
    bool clearUndoList();
    bool eraseInvalidRedoItems();

    List m_undoList;
    List m_redoList;
};

QDebug operator<<(QDebug debug, const History &history);
