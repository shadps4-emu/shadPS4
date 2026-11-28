// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core {

struct RmapNode {
    RmapNode* parent{};
    RmapNode* left{};
    RmapNode* right{};
    RmapNode* prev{};
    RmapNode* next{};
    RmapNode* _next{};
    VAddr vaddr{};
    s32 p_start_idx{};
    s32 p_end_idx{};
    s32 p_adj_free_idx{};
};

template <typename Traits>
class RmapSplayTree {
public:
    using Entry = typename Traits::EntryType;

private:
    using N = RmapNode;

    static N* GetNode(Entry* entry) {
        return Traits::GetNode(entry);
    }
    static Entry* GetEntry(N* node) {
        return Traits::FromNode(node);
    }

public:
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
            auto t = *this;
            m_node = m_node->next;
            return t;
        }
        Iterator& operator--() {
            m_node = m_node->prev;
            return *this;
        }
        Iterator operator--(int) {
            auto t = *this;
            m_node = m_node->prev;
            return t;
        }

        bool operator==(const Iterator& o) const {
            return m_node == o.m_node;
        }
        bool operator!=(const Iterator& o) const {
            return m_node != o.m_node;
        }

        N* node() const {
            return m_node;
        }

    private:
        N* m_node{};
    };

    using iterator = Iterator;

    /// Overlap list iterator — walks the _next chain from FindOverlapping.
    /// Pre-caches m_next so that Remove/modify during iteration is safe.
    class OverlapIterator {
    public:
        using value_type = Entry;
        using pointer = Entry*;
        using reference = Entry&;

        OverlapIterator() = default;
        explicit OverlapIterator(N* node) : m_node{node}, m_next{node ? node->_next : nullptr} {}

        reference operator*() const {
            return *GetEntry(m_node);
        }
        pointer operator->() const {
            return GetEntry(m_node);
        }

        OverlapIterator& operator++() {
            m_node = m_next;
            m_next = m_node ? m_node->_next : nullptr;
            return *this;
        }

        bool operator==(const OverlapIterator& o) const {
            return m_node == o.m_node;
        }
        bool operator!=(const OverlapIterator& o) const {
            return m_node != o.m_node;
        }

        N* node() const {
            return m_node;
        }

    private:
        N* m_node{};
        N* m_next{};
    };

    /// Range for iterating overlaps via range-based for.
    struct OverlapRange {
        N* head;
        OverlapIterator begin() const {
            return OverlapIterator{head};
        }
        OverlapIterator end() const {
            return OverlapIterator{nullptr};
        }
    };

private:
    static int CompareKeys(s32 a_idx, VAddr a_va, s32 b_idx, VAddr b_va) {
        if (a_idx < b_idx)
            return -1;
        if (a_idx > b_idx)
            return 1;
        if (static_cast<s64>(a_va) < static_cast<s64>(b_va))
            return -1;
        if (static_cast<s64>(a_va) > static_cast<s64>(b_va))
            return 1;
        return 0;
    }

    static int CompareToNode(s32 idx, VAddr va, N* node) {
        return CompareKeys(idx, va, node->p_start_idx, node->vaddr);
    }

    void UpdateAug(N* node) {
        s32 val = node->p_end_idx;
        if (node->left != &m_nil && node->left->p_adj_free_idx > val)
            val = node->left->p_adj_free_idx;
        if (node->right != &m_nil && node->right->p_adj_free_idx > val)
            val = node->right->p_adj_free_idx;
        node->p_adj_free_idx = val;
    }

    void SplayInternal(N* root, N* node) {
        if (root == node)
            return;

        N* parent = node->parent;
        if (parent != root) {
            N* left_child = parent->left;
            N* ggp;
            do {
                N* gp = parent->parent;
                ggp = gp->parent;
                N* rotated = node;

                if (left_child == node) {
                    N* gp_left = gp->left;
                    parent->left = node->right;
                    node->right->parent = parent;
                    node->right = parent;
                    parent->parent = node;
                    N** gp_slot;
                    N** node_slot;
                    if (gp_left == parent) {
                        gp_slot = &gp->left;
                        node_slot = &parent->right;
                        rotated = parent;
                    } else {
                        gp_slot = &gp->right;
                        node_slot = &node->left;
                    }
                    *gp_slot = *node_slot;
                    (*node_slot)->parent = gp;
                    *node_slot = gp;
                    gp->parent = rotated;
                } else {
                    N* gp_right = gp->right;
                    parent->right = node->left;
                    node->left->parent = parent;
                    node->left = parent;
                    parent->parent = node;
                    N** gp_slot;
                    N** node_slot;
                    if (gp_right == parent) {
                        gp_slot = &gp->right;
                        node_slot = &parent->left;
                        rotated = parent;
                    } else {
                        gp_slot = &gp->left;
                        node_slot = &node->right;
                    }
                    *gp_slot = *node_slot;
                    (*node_slot)->parent = gp;
                    *node_slot = gp;
                    gp->parent = rotated;
                }

                node->p_adj_free_idx = gp->p_adj_free_idx;
                UpdateAug(gp);
                UpdateAug(parent);
                node->parent = ggp;

                if (gp == root)
                    return;

                N* ggp_left = ggp->left;
                left_child = ggp_left;
                if (ggp_left == gp) {
                    left_child = node;
                    ggp->left = node;
                } else {
                    ggp->right = node;
                }
                parent = ggp;
            } while (ggp != root);
        }

        if (root->left == node) {
            N* nr = node->right;
            root->left = nr;
            nr->parent = root;
            node->right = root;
        } else {
            N* nl = node->left;
            root->right = nl;
            nl->parent = root;
            node->left = root;
        }
        node->parent = root->parent;
        root->parent = node;
        node->p_adj_free_idx = root->p_adj_free_idx;
        UpdateAug(root);
    }

    void SplayToRoot(N* node) {
        SplayInternal(m_root, node);
        m_root = node;
    }

    iterator make_iter(N* node) const {
        return iterator(node);
    }

public:
    RmapSplayTree() {
        Init();
    }

    void Init() {
        m_nil = {};
        m_nil.parent = &m_nil;
        m_nil.left = &m_nil;
        m_nil.right = &m_nil;
        m_nil.prev = &m_nil;
        m_nil.next = &m_nil;
        m_nil.p_adj_free_idx = 0;

        m_head = {};
        m_head.parent = &m_nil;
        m_head.left = &m_nil;
        m_head.right = &m_tail;
        m_head.next = &m_tail;
        m_head.prev = &m_tail;
        m_head.p_start_idx = 0;
        m_head.p_end_idx = 0;
        m_head.p_adj_free_idx = 0x7fffffff;

        m_tail = {};
        m_tail.parent = &m_head;
        m_tail.left = &m_nil;
        m_tail.right = &m_nil;
        m_tail.next = &m_head;
        m_tail.prev = &m_head;
        m_tail.p_start_idx = 0x7fffffff;
        m_tail.p_end_idx = 0x7fffffff;
        m_tail.p_adj_free_idx = 0x7fffffff;

        m_root = &m_head;
    }

    iterator begin() const {
        return iterator(m_head.next);
    }
    iterator end() const {
        return iterator(const_cast<N*>(&m_tail));
    }
    bool IsEmpty() const {
        return m_head.next == &m_tail;
    }

    /// Find nearest entry to (page_idx, vaddr). Returns end() if tree is empty.
    iterator Find(s32 page_idx, VAddr vaddr) {
        N* cur = m_root;
        N* result = cur;
        while (true) {
            result = cur;
            int cmp = CompareToNode(page_idx, vaddr, cur);
            if (cmp < 0) {
                cur = cur->left;
            } else if (cmp > 0) {
                cur = cur->right;
            } else {
                return make_iter(result);
            }
            if (cur == &m_nil) {
                return make_iter(result);
            }
        }
    }

    void Splay(OverlapIterator it) {
        SplayToRoot(it.node());
    }
    void Splay(iterator it) {
        SplayToRoot(it.node());
    }

    iterator Insert(Entry* entry) {
        N* node = GetNode(entry);
        // Find nearest and splay to root.
        auto nearest = Find(node->p_start_idx, node->vaddr);
        Splay(nearest);
        N* root = m_root;

        int cmp = CompareKeys(root->p_start_idx, root->vaddr, node->p_start_idx, node->vaddr);
        if (cmp < 0) {
            node->next = root->next;
            root->next->prev = node;
            node->prev = root;
            root->next = node;
            node->right = root->right;
            root->right->parent = node;
            root->right = &m_nil;
            s32 aug = root->left->p_adj_free_idx;
            if (aug < root->p_end_idx)
                aug = root->p_end_idx;
            root->p_adj_free_idx = aug;
            node->left = root;
        } else {
            node->prev = root->prev;
            root->prev->next = node;
            node->next = root;
            root->prev = node;
            node->left = root->left;
            root->left->parent = node;
            root->left = &m_nil;
            s32 aug = root->right->p_adj_free_idx;
            if (aug < root->p_end_idx)
                aug = root->p_end_idx;
            root->p_adj_free_idx = aug;
            node->right = root;
        }
        root->parent = node;
        s32 aug = root->p_adj_free_idx;
        if (aug < node->p_end_idx)
            aug = node->p_end_idx;
        node->p_adj_free_idx = aug;
        node->parent = &m_nil;
        m_root = node;
        return make_iter(node);
    }

    iterator Remove(iterator it) {
        return RemoveNode(it.node());
    }
    iterator Remove(OverlapIterator it) {
        return RemoveNode(it.node());
    }

    /// Remove entry, returns iterator to the next entry.
    iterator RemoveNode(N* node) {
        N* next_node = node->next;
        SplayToRoot(node);

        node->prev->next = node->next;
        node->next->prev = node->prev;

        if (node->left == &m_nil) {
            m_root = node->right;
            m_root->parent = &m_nil;
        } else if (node->right == &m_nil) {
            m_root = node->left;
            m_root->parent = &m_nil;
        } else {
            N* successor = node->next;
            SplayInternal(node->right, successor);
            successor->left = node->left;
            node->left->parent = successor;
            successor->parent = &m_nil;
            UpdateAug(successor);
            m_root = successor;
        }

        node->left = nullptr;
        node->right = nullptr;
        node->parent = nullptr;
        node->prev = nullptr;
        node->next = nullptr;
        return iterator(next_node);
    }

    /// Find all entries whose physical page range overlaps [start_page, end_page)
    /// and whose phys-to-virt offset matches the caller's range.
    /// Returns OverlapRange for range-based for. Also returns remaining
    /// uncovered page count via out parameter (0 = all pages accounted for).
    OverlapRange FindOverlapping(s32 start_page, s32 end_page, VAddr vaddr_start,
                                 s32* remaining_out = nullptr) {
        s32 remaining = end_page - start_page;
        N* hit_list = nullptr;

        if (IsEmpty()) {
            if (remaining_out) {
                *remaining_out = remaining;
            }
            return {nullptr};
        }

        const s64 expected_offset =
            static_cast<s64>(start_page) - static_cast<s64>(vaddr_start >> 14);

        N* cursor = m_root;
        N* stack = nullptr;

        // Kernel loop: while(true) { do { while(true) { ... } } while(...); if(!stack) break; }
        // Flattened into a single loop with explicit stack management.
        while (true) {
            // Inner: descend and check nodes.
            while (true) {
                const s32 p_start = cursor->p_start_idx;

                // Overlap + offset check (kernel lines 895-908).
                if (p_start < end_page) {
                    const s64 entry_offset =
                        static_cast<s64>(p_start) - static_cast<s64>(cursor->vaddr >> 14);
                    if (entry_offset == expected_offset) {
                        const s32 p_end = cursor->p_end_idx;
                        if (start_page < p_end) {
                            // Hit — add to list.
                            cursor->_next = hit_list;
                            hit_list = cursor;

                            // Update remaining pages.
                            s32 clamped_end = (end_page < p_end) ? end_page : p_end;
                            s32 clamped_start = (start_page <= p_start) ? p_start : start_page;
                            remaining += (clamped_start - clamped_end);

                            if (remaining < 1) {
                                // All pages accounted for — stop early.
                                if (remaining_out)
                                    *remaining_out = remaining;
                                return {hit_list};
                            }
                        }
                    }
                }

                // Try descend left (kernel lines 910-917).
                N* right_child = cursor->right;
                if (cursor->left != &m_nil && cursor->left->p_adj_free_idx > start_page) {
                    // Left subtree might have overlaps — descend left.
                    // Push right child onto deferred stack if it's not nil.
                    if (right_child != &m_nil) {
                        right_child->_next = stack;
                        stack = right_child;
                    }
                    cursor = cursor->left;
                } else {
                    // Left pruned — try right (kernel lines 918-920).
                    cursor = right_child;
                    break;
                }
            }

            // Outer: if right child was nil (== header/sentinel), pop from stack.
            // Kernel: do { ... } while (next2 != pvVar1); then pop.
            while (cursor == &m_nil) {
                if (stack == nullptr) {
                    if (remaining_out)
                        *remaining_out = remaining;
                    return {hit_list};
                }
                // Pop from stack — do NOT clear _next (matches kernel line 922-923).
                cursor = stack;
                stack = stack->_next;
            }
        }
    }

    /// Simplified FindOverlapping without offset check.
    OverlapRange FindOverlappingAll(s32 start_page, s32 end_page) {
        N* hit_list = nullptr;

        if (IsEmpty()) {
            return {nullptr};
        }

        N* cursor = m_root;
        N* stack = nullptr;

        while (true) {
            while (true) {
                if (cursor->p_start_idx < end_page && start_page < cursor->p_end_idx) {
                    cursor->_next = hit_list;
                    hit_list = cursor;
                }

                N* right_child = cursor->right;
                if (cursor->left != &m_nil && cursor->left->p_adj_free_idx > start_page) {
                    if (right_child != &m_nil) {
                        right_child->_next = stack;
                        stack = right_child;
                    }
                    cursor = cursor->left;
                } else {
                    cursor = right_child;
                    break;
                }
            }

            while (cursor == &m_nil) {
                if (stack == nullptr) {
                    return {hit_list};
                }
                cursor = stack;
                stack = stack->_next;
            }
        }
    }

    void UpdateAug(OverlapIterator it) {
        UpdateAug(it.node());
    }

    N* Nil() {
        return &m_nil;
    }
    N* Root() {
        return m_root;
    }

private:
    N m_nil{};
    N m_head{};
    N m_tail{};
    N* m_root{};
};

template <typename T>
struct InheritRmapTraits {
    using EntryType = T;
    static RmapNode* GetNode(T* entry) {
        return static_cast<RmapNode*>(entry);
    }
    static T* FromNode(RmapNode* node) {
        return static_cast<T*>(node);
    }
};

} // namespace Core