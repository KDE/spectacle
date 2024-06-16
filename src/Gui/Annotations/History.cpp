/* SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "History.h"
#include <QDebug>
#include <ranges>

using namespace Qt::StringLiterals;

bool HistoryItem::hasParent() const
{
    return m_parent && !m_parent->expired();
}

HistoryItem::const_weak_ptr HistoryItem::parent() const
{
    return m_parent.value_or(const_weak_ptr{});
}

bool HistoryItem::hasChild() const
{
    return !m_child.expired();
}

HistoryItem::const_weak_ptr HistoryItem::child() const
{
    return m_child;
}

const Traits::OptTuple &HistoryItem::traits() const
{
    return m_traits;
}

Traits::OptTuple &HistoryItem::traits()
{
    return m_traits;
}

bool HistoryItem::isValid() const
{
    return Traits::isValid(m_traits) && (!m_parent || !m_parent->expired());
}

bool HistoryItem::visibleTraits() const
{
    return Traits::isVisible(m_traits);
}

QRectF HistoryItem::renderRect() const
{
    auto visualRect = Traits::visualRect(m_traits);
    if (visualRect.isEmpty() && hasParent()) {
        auto parent = m_parent->lock();
        return parent ? parent->renderRect() : visualRect;
    } else {
        return visualRect;
    }
}

void HistoryItem::setItemRelations(shared_ptr parent, shared_ptr child)
{
    if (child) {
        child->m_parent = parent;
    }
    if (parent) {
        parent->m_child = child;
    }
}
void HistoryItem::setItemRelations(const_shared_ptr parent, const_shared_ptr child)
{
    if (child) {
        child->m_parent = parent;
    }
    if (parent) {
        parent->m_child = child;
    }
}
void HistoryItem::setItemRelations(shared_ptr parent, const_shared_ptr child)
{
    if (child) {
        child->m_parent = parent;
    }
    if (parent) {
        parent->m_child = child;
    }
}
void HistoryItem::setItemRelations(const_shared_ptr parent, shared_ptr child)
{
    if (child) {
        child->m_parent = parent;
    }
    if (parent) {
        parent->m_child = child;
    }
}

QDebug operator<<(QDebug debug, const HistoryItem &item)
{
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << "HistoryItem(";
    debug << (const void *)&item;
    auto parent = item.m_parent.value_or(HistoryItem::const_weak_ptr{}).lock().get();
    debug << ",\n    parent=" << (const void *)parent;
    auto child = item.m_child.lock().get();
    debug << ",\n    child=" << (const void *)child;
    debug << ",\n    isValid=" << item.isValid();
    debug << ",\n    visibleTraits=" << item.visibleTraits();
    debug << ",\n    renderRect=" << item.renderRect();
    debug << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const HistoryItem *item)
{
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    if (item) {
        debug << *item;
    } else {
        debug << "HistoryItem(0x0)";
    }
    return debug;
}

//---

template<typename It>
    requires(std::same_as<It, History::List::iterator> || std::same_as<It, History::List::reverse_iterator>)
History::List::size_type eraseInvalidItems(History::List &list, It first, It last)
{
    if (first == last) {
        return 0;
    }
    History::List::size_type removedCount = 0;
    for (auto it = first; it != last; ++it) {
        auto &item = *it;
        if (!item || !item->isValid()) {
            // std::next(it).base() makes it safe to erase with reverse iterators.
            if constexpr (std::same_as<It, History::List::reverse_iterator>) {
                list.erase(std::next(it).base());
            } else {
                list.erase(it);
            }
            ++removedCount;
        }
    }
    return removedCount;
}

//---

History::History(const List &undoList, const List &redoList)
    : m_undoList(undoList)
    , m_redoList(redoList)
{
}

bool History::operator==(const History &other) const
{
    return m_undoList == other.m_undoList && m_redoList == other.m_redoList;
}

static inline HistoryItem::const_shared_ptr castToImmutable(const HistoryItem::shared_ptr &item)
{
    return std::static_pointer_cast<const HistoryItem, HistoryItem>(item);
}

History::ImmutableView History::undoList() const
{
    return std::ranges::transform_view{RefView{m_undoList}, &castToImmutable};
}

History::ImmutableView History::redoList() const
{
    return std::ranges::transform_view{RefView{m_redoList}, &castToImmutable};
}

History::List::size_type History::currentIndex() const
{
    return m_undoList.size() - 1;
}

HistoryItem::shared_ptr History::currentItem() const
{
    if (!m_undoList.empty()) {
        return m_undoList.back();
    }
    return {};
}

History::Lists History::filteredLists(const std::function<bool(History::List::const_reference)> &function) const
{
    History::Lists lists;
    for (auto it = m_undoList.cbegin(); it != m_undoList.cend(); ++it) {
        if (function(*it)) {
            lists.undoList.push_back(*it);
        }
    }
    for (auto it = m_redoList.cbegin(); it != m_redoList.cend(); ++it) {
        if (function(*it)) {
            lists.redoList.push_back(*it);
        }
    }
    return lists;
}

History::ListsChangedResult History::push(const HistoryItem::shared_ptr &item)
{
    if (!item) {
        return {false, false};
    }
    if (!m_undoList.empty() && (!m_undoList.back() || !m_undoList.back()->isValid())) {
        m_undoList.pop_back();
    }
    m_undoList.push_back(item);
    return {true, clearRedoList()};
}

History::ItemReplacedResult History::pop()
{
    if (m_undoList.empty()) {
        return {nullptr, false};
    }
    auto item = std::move(m_undoList.back());
    m_undoList.erase(m_undoList.cend() - 1);
    return {item, eraseInvalidRedoItems()};
}

bool History::undo()
{
    if (m_undoList.empty()) {
        return false;
    }
    m_redoList.push_back(std::move(m_undoList.back()));
    m_undoList.erase(m_undoList.cend() - 1);
    return true;
}

bool History::redo()
{
    if (m_redoList.empty()) {
        return false;
    }
    m_undoList.push_back(std::move(m_redoList.back()));
    m_redoList.erase(m_redoList.cend() - 1);
    return true;
}

bool History::clearRedoList()
{
    if (m_redoList.empty()) {
        return false;
    }
    const auto oldSize = m_redoList.size();
    while (!m_redoList.empty()) {
        m_redoList.erase(m_redoList.cend() - 1);
    }
    return oldSize != m_redoList.size();
}

bool History::clearUndoList()
{
    if (m_undoList.empty()) {
        return false;
    }
    const auto oldSize = m_undoList.size();
    while (!m_undoList.empty()) {
        m_undoList.erase(m_undoList.cend() - 1);
    }
    return oldSize != m_undoList.size();
}

History::ListsChangedResult History::clearLists()
{
    return {clearUndoList(), clearRedoList()};
}

bool History::itemVisible(const HistoryItem::const_shared_ptr &item) const
{
    if (!item || !item->visibleTraits()) {
        return false;
    }
    auto child = item->m_child.lock();
    // Searching in reverse order should be a bit faster.
    // If the returned iterator is equal to crend, then the child wasn't found.
    return !child || std::find(m_undoList.crbegin(), m_undoList.crend(), child) == m_undoList.crend();
}

bool History::eraseInvalidRedoItems()
{
    // Erase in chronological order so that later item Parent traits become invalidated.
    return eraseInvalidItems(m_redoList, m_redoList.rbegin(), m_redoList.rend());
}

QDebug operator<<(QDebug debug, const History &history)
{
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << "History(";
    debug << (const void *)&history;
    debug << ",\n  undoList.size()=" << history.m_undoList.size();
    debug << ",\n  redoList.size()=" << history.m_redoList.size();
    debug << ')';
    return debug;
}
