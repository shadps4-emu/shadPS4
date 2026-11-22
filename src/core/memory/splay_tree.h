// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/alignment.h"
#include "common/assert.h"
#include "common/types.h"

namespace Core {

struct SplayTreeNode {
    SplayTreeNode* left{};
    SplayTreeNode* right{};
    SplayTreeNode* prev{};
    SplayTreeNode* next{};
    SplayTreeNode* temp{};
    u64 adj_free{};
    u64 max_free{};
};

template <typename Traits>
class SplayTree {
public:
    using Entry = typename Traits::EntryType;

private:
    using N = SplayTreeNode;

    static N* GetNode(Entry* entry) {
        return Traits::GetNode(entry);
    }
    static Entry* GetEntry(N* node) {
        return Traits::FromNode(node);
    }
    static u64 Start(Entry* entry) {
        return Traits::GetStart(entry);
    }
    static u64 End(Entry* entry) {
        return Traits::GetEnd(entry);
    }
    static u64 Start(N* node) {
        return Start(GetEntry(node));
    }
    static u64 End(N* node) {
        return End(GetEntry(node));
    }

public:
    /// Bidirectional iterator over the linked list.
    /// end() points to the header sentinel. Dereferencing end() is UB.
    /// Comparing with end() is always safe.
    class Iterator {
    public:
        using value_type = Entry;
        using pointer = Entry*;
        using reference = Entry&;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;

        Iterator() = default;
        explicit Iterator(N* node) : m_node{node} {}

        reference operator*() const {
            return *GetEntry(m_node);
        }
        pointer operator->() const {
            return GetEntry(m_node);
        }

        Iterator& operator++() {
            m_node = m_node->next;
            return *this;
        }
        Iterator operator++(int) {
            auto tmp = *this;
            m_node = m_node->next;
            return tmp;
        }
        Iterator& operator--() {
            m_node = m_node->prev;
            return *this;
        }
        Iterator operator--(int) {
            auto tmp = *this;
            m_node = m_node->prev;
            return tmp;
        }

        bool operator==(const Iterator& o) const {
            return m_node == o.m_node;
        }
        bool operator!=(const Iterator& o) const {
            return m_node != o.m_node;
        }

        /// Get the underlying node pointer. Useful for passing to Link/Unlink.
        N* node() const {
            return m_node;
        }

    private:
        N* m_node{};
        friend class SplayTree;
    };

    using iterator = Iterator;

private:
    static void SetMaxFree(N* entry) {
        entry->max_free = entry->adj_free;
        if (entry->left && entry->left->max_free > entry->max_free)
            entry->max_free = entry->left->max_free;
        if (entry->right && entry->right->max_free > entry->max_free)
            entry->max_free = entry->right->max_free;
    }

    static N* Splay(u64 addr, N* root) {
        if (!root)
            return root;
        N *llist = nullptr, *rlist = nullptr, *ltree, *rtree, *y;
        for (;;) {
            if (addr < Start(root)) {
                y = root->left;
                if (!y)
                    break;
                if (addr < Start(y) && y->left) {
                    root->left = y->right;
                    y->right = root;
                    SetMaxFree(root);
                    root = y->left;
                    y->left = rlist;
                    rlist = y;
                } else {
                    root->left = rlist;
                    rlist = root;
                    root = y;
                }
            } else if (addr >= End(root)) {
                y = root->right;
                if (!y)
                    break;
                if (addr >= End(y) && y->right) {
                    root->right = y->left;
                    y->left = root;
                    SetMaxFree(root);
                    root = y->right;
                    y->right = llist;
                    llist = y;
                } else {
                    root->right = llist;
                    llist = root;
                    root = y;
                }
            } else
                break;
        }
        ltree = root->left;
        while (llist) {
            y = llist->right;
            llist->right = ltree;
            SetMaxFree(llist);
            ltree = llist;
            llist = y;
        }
        rtree = root->right;
        while (rlist) {
            y = rlist->left;
            rlist->left = rtree;
            SetMaxFree(rlist);
            rtree = rlist;
            rlist = y;
        }
        root->left = ltree;
        root->right = rtree;
        SetMaxFree(root);
        return root;
    }

    static N* SplayMaxFree(N* root) {
        if (!root)
            return root;
        u64 target = root->max_free;
        if (root->adj_free == target)
            return root;
        N *llist = nullptr, *rlist = nullptr;
        for (;;) {
            N* left = root->left;
            if (left && left->max_free == target) {
                if (left->adj_free != target && left->left && left->left->max_free == target) {
                    root->left = left->right;
                    left->right = root;
                    SetMaxFree(root);
                    root = left->left;
                    left->left = llist;
                    llist = left;
                } else {
                    root->left = llist;
                    llist = root;
                    root = left;
                }
            } else {
                N* right = root->right;
                if (right->adj_free != target && right->right && right->right->max_free == target) {
                    root->right = right->left;
                    right->left = root;
                    SetMaxFree(root);
                    root = right->right;
                    right->right = rlist;
                    rlist = right;
                } else {
                    root->right = rlist;
                    rlist = root;
                    root = right;
                }
            }
            if (root->adj_free == target)
                break;
        }
        N* ltree = root->left;
        while (llist) {
            N* y = llist->right;
            llist->right = ltree;
            SetMaxFree(llist);
            ltree = llist;
            llist = y;
        }
        N* rtree = root->right;
        while (rlist) {
            N* y = rlist->left;
            rlist->left = rtree;
            SetMaxFree(rlist);
            rtree = rlist;
            rlist = y;
        }
        root->left = ltree;
        root->right = rtree;
        SetMaxFree(root);
        return root;
    }

    /// Create an iterator from a node. Returns end() if node is header.
    iterator make_iter(N* node) const {
        return iterator(node);
    }

public:
    SplayTree() = default;

    void Init(Entry* header_entry, u64 min_offset, u64 max_offset) {
        m_header = GetNode(header_entry);
        m_header->prev = m_header;
        m_header->next = m_header;
        m_root = nullptr;
        m_num_entries = 0;
        m_min_offset = min_offset;
    }

    iterator begin() const {
        return iterator(m_header->next);
    }
    iterator end() const {
        return iterator(m_header);
    }

    /// Find the entry containing addr or the entry immediately before it.
    std::pair<iterator, bool> LookupEntry(u64 addr) {
        if (!m_root)
            return {end(), false};
        if (m_root && addr >= Start(m_root) && addr < End(m_root)) {
            return {make_iter(m_root), true};
        }
        m_root = Splay(addr, m_root);
        N* cur = m_root;
        // Debug: verify splay result is actually nearest
        if (addr < Start(cur) && cur->prev != m_header) {
            // cur->prev should be the predecessor with end <= addr
            if (End(cur->prev) > addr) {
                ASSERT_MSG(false,
                           "splay landed wrong: addr={:#x} cur=[{:#x},{:#x}) prev=[{:#x},{:#x})",
                           addr, Start(cur), End(cur), Start(cur->prev), End(cur->prev));
            }
        }
        if (addr >= End(cur) && cur->next != m_header) {
            // cur->next should be the successor with start > addr
            ASSERT_MSG(Start(cur->next) > addr,
                       "splay landed wrong: addr={:#x} cur=[{:#x},{:#x}) next=[{:#x},{:#x})", addr,
                       Start(cur), End(cur), Start(cur->next), End(cur->next));
        }

        if (addr >= Start(cur)) {
            return {make_iter(cur), addr < End(cur)};
        } else {
            return {make_iter(cur->prev), false};
        }
    }

    /// Non-splaying lookup safe for concurrent reads.
    std::pair<iterator, bool> LookupEntryReadOnly(u64 addr) const {
        N* cur = m_root;
        if (!cur) {
            return {end(), false};
        }

        if (addr >= Start(cur) && addr < End(cur)) {
            return {make_iter(cur), true};
        }

        for (;;) {
            if (addr < Start(cur)) {
                if (!cur->left) {
                    return {make_iter(cur->prev), false};
                }
                cur = cur->left;
            } else if (addr < End(cur)) {
                return {make_iter(cur), true};
            } else {
                if (!cur->right) {
                    return {make_iter(cur), false};
                }
                cur = cur->right;
            }
        }
    }

    /// Find first fit for length free bytes at address >= start.
    VAddr FindSpace(u64 start, u64 length) {
        start = std::max(start, m_min_offset);
        if (!m_root) {
            return start;
        }
        m_root = Splay(start, m_root);
        if (start + length <= Start(m_root)) {
            return start;
        }
        u64 sb = std::max(start, End(m_root));
        if (length <= End(m_root) + m_root->adj_free - sb) {
            return sb;
        }
        N* node = m_root->right;
        if (!node || length > node->max_free) {
            return 0;
        }
        while (node) {
            if (node->left && node->left->max_free >= length) {
                node = node->left;
            } else if (node->adj_free >= length) {
                return End(node);
            } else {
                node = node->right;
            }
        }
        return 0;
    }

    /// Find first aligned fit for length free bytes at address >= start.
    std::pair<iterator, VAddr> FindSpaceAligned(u64 start, u64 search_end, u64 length,
                                                u64 alignment) {
        const u64 upper = Common::AlignDownPow2(search_end - length, alignment) + 1;
        for (;;) {
            const u64 sa = Common::AlignUpPow2(start, alignment);
            if (start > sa || sa >= upper) {
                return {end(), -1};
            }

            m_root = Splay(sa, m_root);
            if (sa + length <= Start(m_root)) {
                return {make_iter(m_root->prev), sa};
            }

            const u64 sb = std::max(sa, End(m_root));
            const u64 sba = Common::AlignUpPow2(sb, alignment);
            if (sb > sba || sba >= upper) {
                return {end(), -1};
            }

            if (s64(length) <= s64(End(m_root) + m_root->adj_free - sba)) {
                return {make_iter(m_root), sba};
            }

            N* node = m_root->right;
            if (!node || length > node->max_free) {
                return {end(), -1};
            }

            while (node) {
                if (node->left && node->left->max_free >= length) {
                    node = node->left;
                } else if (node->adj_free >= length) {
                    break;
                } else {
                    node = node->right;
                }
            }
            if (!node) {
                return {end(), -1};
            }

            const u64 addr = std::max(End(node), sba);
            const u64 aa = Common::AlignUpPow2(addr, alignment);
            if (addr > aa || aa >= upper) {
                return {end(), -1};
            }

            start = End(node) + node->adj_free;
            if (aa + length > start) {
                continue;
            }

            return {make_iter(node), aa};
        }
    }

    /// Insert entry after `pos`. pos may be end() (header) to insert at front.
    iterator Link(iterator pos, Entry* entry) {
        N* after = pos.m_node;
        N* node = GetNode(entry);

        m_num_entries++;
        node->prev = after;
        node->next = after->next;
        node->next->prev = node;
        after->next = node;

        if (after != m_header) {
            if (after != m_root)
                m_root = Splay(Start(after), m_root);
            node->right = after->right;
            node->left = after;
            after->right = nullptr;
            after->adj_free = Start(node) - End(after);
            SetMaxFree(after);
        } else {
            node->right = m_root;
            node->left = nullptr;
        }
        node->adj_free = Start(node->next) - End(node);
        SetMaxFree(node);
        m_root = node;

        return iterator(node);
    }

    /// Remove the entry pointed to by `pos`. Returns iterator to the next entry.
    iterator Unlink(iterator pos) {
        N* node = pos.m_node;
        N* next_node = node->next;
        N* root;

        if (node != m_root)
            m_root = Splay(Start(node), m_root);

        if (!node->left) {
            root = node->right;
        } else {
            root = Splay(Start(node), node->left);
            root->right = node->right;
            root->adj_free = Start(node->next) - End(root);
            SetMaxFree(root);
        }
        m_root = root;

        node->next->prev = node->prev;
        node->prev->next = node->next;
        m_num_entries--;

        node->left = nullptr;
        node->right = nullptr;
        node->prev = nullptr;
        node->next = nullptr;

        return iterator(next_node);
    }

    /// Remove the predecessor of entry from the tree and list.
    void UnlinkPrev(iterator entry) {
        N* entry_node = entry.node();
        N* prev_node = entry_node->prev;
        m_root = Splay(Start(prev_node), m_root);

        prev_node->right = Splay(Start(entry_node), prev_node->right);
        entry_node->left = prev_node->left;
        m_root = entry_node;

        entry_node->adj_free = Start(entry_node->next) - End(entry_node);
        SetMaxFree(entry_node);

        // Unlink prev from list.
        prev_node->prev->next = prev_node->next;
        prev_node->next->prev = prev_node->prev;

        prev_node->left = nullptr;
        prev_node->right = nullptr;
        prev_node->prev = nullptr;
        prev_node->next = nullptr;

        m_num_entries--;
    }

    /// Recompute adj_free/max_free after an in-place resize.
    void ResizeFree(iterator pos) {
        N* node = pos.m_node;
        if (node != m_root) {
            m_root = Splay(Start(node), m_root);
        }
        node->adj_free = Start(node->next) - End(node);
        SetMaxFree(node);
    }

    Entry* SubtreeSplay(Entry* subtree_root, u64 addr) {
        return GetEntry(Splay(addr, GetNode(subtree_root)));
    }
    Entry* SubtreeSplayMaxFree(Entry* subtree_root) {
        return GetEntry(SplayMaxFree(GetNode(subtree_root)));
    }

    static Entry* Left(Entry* entry) {
        N* left = GetNode(entry)->left;
        return left ? GetEntry(left) : nullptr;
    }
    static Entry* Right(Entry* entry) {
        N* right = GetNode(entry)->right;
        return right ? GetEntry(right) : nullptr;
    }
    static Entry* Prev(Entry* entry) {
        N* prev = GetNode(entry)->prev;
        return prev ? GetEntry(prev) : nullptr;
    }
    static Entry* Temp(Entry* e) {
        N* t = GetNode(e)->temp;
        return t ? GetEntry(t) : nullptr;
    }
    static void SetLeft(Entry* p, Entry* c) {
        GetNode(p)->left = c ? GetNode(c) : nullptr;
        SetMaxFree(GetNode(p));
    }
    static void SetRight(Entry* p, Entry* c) {
        GetNode(p)->right = c ? GetNode(c) : nullptr;
        SetMaxFree(GetNode(p));
    }
    static void SetTemp(Entry* e, Entry* v) {
        GetNode(e)->temp = v ? GetNode(v) : nullptr;
    }
    static u64 AdjFree(Entry* e) {
        return GetNode(e)->adj_free;
    }
    static u64 MaxFree(Entry* e) {
        return GetNode(e)->max_free;
    }

    bool IsEmpty() const {
        return m_num_entries == 0;
    }
    u32 Count() const {
        return m_num_entries;
    }
    Entry* Root() const {
        return m_root ? GetEntry(m_root) : nullptr;
    }

private:
    N* m_header{};
    N* m_root{};
    u32 m_num_entries{};
    u64 m_min_offset;
};

template <typename T>
struct InheritSplayTraits {
    using EntryType = T;
    static SplayTreeNode* GetNode(T* e) {
        return static_cast<SplayTreeNode*>(e);
    }
    static T* FromNode(SplayTreeNode* n) {
        return static_cast<T*>(n);
    }
    static u64 GetStart(const T* e) {
        return e->start;
    }
    static u64 GetEnd(const T* e) {
        return e->end;
    }
};

template <typename T, SplayTreeNode T::* Member>
struct MemberSplayTraits {
    using EntryType = T;
    static SplayTreeNode* GetNode(T* e) {
        return &(e->*Member);
    }
    static T* FromNode(SplayTreeNode* n) {
        auto Off = reinterpret_cast<uintptr_t>(&(static_cast<T*>(nullptr)->*Member));
        return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(n) - Off);
    }
    static u64 GetStart(const T* e) {
        return e->start;
    }
    static u64 GetEnd(const T* e) {
        return e->end;
    }
};

} // namespace Core