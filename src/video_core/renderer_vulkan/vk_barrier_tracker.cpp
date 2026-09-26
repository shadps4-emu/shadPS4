// SPDX-FileCopyrightText: Copyright DXVK Project
// SPDX-License-Identifier: Zlib

#include <bit>
#include "video_core/renderer_vulkan/vk_barrier_tracker.h"

namespace Vulkan {

BarrierTracker::BarrierTracker() {
    // Having an accessible 0 node makes certain things easier to
    // implement and allows us to use 0 as an invalid node index.
    nodes.emplace_back();

    // Pre-allocate root nodes for the implicit hash table
    for (u32 i = 0; i < 2u * HashTableSize; i++) {
        AllocateNode();
    }
}

bool BarrierTracker::FindRange(const AddressRange& range, Access access_type) const {
    u32 root_index = ComputeRootIndex(range, access_type);
    u32 node_index = FindNode(range, root_index);
    return node_index;
}

void BarrierTracker::InsertRange(const AddressRange& range, Access access_type) {
    // If we can just insert the node with no conflicts, we don't have to do anything.
    u32 root_index = ComputeRootIndex(range, access_type);
    u32 node_index = InsertNode(range, root_index);

    if (!node_index) [[likely]] {
        return;
    }

    // If there's an existing node and it contains the entire range we want to add already, also
    // don't do anything. If there are conflicting access ops, reset it.
    auto& node = nodes[node_index];
    if (node.address_range.Contains(range)) {
        return;
    }

    // Otherwise, check if there are any other overlapping ranges.
    // If that is not the case, simply update the range we found.
    bool has_overlap = false;

    if (range.range_start < node.address_range.range_start) {
        AddressRange test_range;
        test_range.resource = range.resource;
        test_range.range_start = range.range_start;
        test_range.range_end = node.address_range.range_start - 1;
        has_overlap = FindNode(test_range, root_index);
    }

    if (range.range_end > node.address_range.range_end && !has_overlap) {
        AddressRange test_range;
        test_range.resource = range.resource;
        test_range.range_start = node.address_range.range_end + 1;
        test_range.range_end = range.range_end;
        has_overlap = FindNode(test_range, root_index);
    }

    if (!has_overlap) {
        node.address_range.range_start =
            std::min(node.address_range.range_start, range.range_start);
        node.address_range.range_end = std::max(node.address_range.range_end, range.range_end);
        return;
    }

    // If there are multiple ranges overlapping the one being inserted, remove them all and insert
    // the merged range.
    AddressRange merged_range = range;
    while (node_index) {
        auto& node = nodes[node_index];
        merged_range.range_start =
            std::min(merged_range.range_start, node.address_range.range_start);
        merged_range.range_end = std::max(merged_range.range_end, node.address_range.range_end);

        RemoveNode(node_index, root_index);
        node_index = FindNode(range, root_index);
    }
    InsertNode(merged_range, root_index);
}

void BarrierTracker::Clear() {
    root_mask_valid = 0;
    while (root_mask_subtree) {
        // Free subtrees if any, but keep the root node intact
        const u32 root_index = std::countr_zero(root_mask_subtree) + 1;
        auto& root = nodes[root_index];
        if (root.header) {
            FreeNode(root.Child(0));
            FreeNode(root.Child(1));
            root.header = 0;
        }
        root_mask_subtree &= root_mask_subtree - 1;
    }
}

u32 BarrierTracker::AllocateNode() {
    if (!free.empty()) {
        const u32 node_index = free.back();
        free.pop_back();

        // Free any subtree that the node might still have
        auto& node = nodes[node_index];
        FreeNode(node.Child(0));
        FreeNode(node.Child(1));

        node.header = 0;
        return node_index;
    } else {
        // Allocate entirely new node in the array
        const u32 node_index = nodes.size();
        nodes.emplace_back();
        return node_index;
    }
}

void BarrierTracker::FreeNode(u32 node) {
    if (node) {
        free.push_back(node);
    }
}

u32 BarrierTracker::FindNode(const AddressRange& range, u32 root_index) const {
    // Check if the given root is valid at all
    const u64 root_bit = 1ULL << (root_index - 1);
    if (!(root_mask_valid & root_bit)) {
        return false;
    }

    // Traverse search tree normally
    u32 node_index = root_index;
    while (node_index) {
        auto& node = nodes[node_index];
        if (node.address_range.Overlaps(range)) {
            return node_index;
        }
        node_index = node.Child(node.address_range < range);
    }
    return 0;
}

u32 BarrierTracker::InsertNode(const AddressRange& range, u32 root_index) {
    // Check if the given root is valid at all
    const u64 root_bit = 1ULL << (root_index - 1);

    if (!(root_mask_valid & root_bit)) {
        // Update root node as necessary. Also reset its red-ness if we set it during deletion.
        root_mask_valid |= root_bit;
        auto& node = nodes[root_index];
        node.header = 0;
        node.address_range = range;
        return 0;
    }

    // Traverse tree and abort if we find any range overlapping the one we're trying to insert.
    u32 parent_index = root_index;
    u32 child_index = 0;

    while (true) {
        auto& parent = nodes[parent_index];
        if (parent.address_range.Overlaps(range)) {
            return parent_index;
        }
        child_index = parent.address_range < range;
        if (!parent.Child(child_index)) {
            break;
        }
        parent_index = parent.Child(child_index);
    }

    // Create and insert new node into the tree
    const u32 node_index = AllocateNode();

    auto& parent = nodes[parent_index];
    parent.SetChild(child_index, node_index);

    auto& node = nodes[node_index];
    node.SetRed(true);
    node.SetParent(parent_index);
    node.address_range = range;

    // Only do the fixup to maintain red-black properties if
    // we haven't marked the root node as red in a deletion.
    if (parent_index != root_index && !nodes[root_index].IsRed()) {
        RebalancePostInsert(node_index, root_index);
    }

    root_mask_subtree |= root_bit;
    return 0;
}

void BarrierTracker::RemoveNode(u32 node_index, u32 root_index) {
    auto& node = nodes[node_index];

    u32 l = node.Child(0);
    u32 r = node.Child(1);

    if (l && r) {
        // Both children are valid. Take the payload from the smallest
        // node in the right subtree and delete that node instead.
        u32 child_index = r;
        while (nodes[child_index].Child(0)) {
            child_index = nodes[child_index].Child(0);
        }

        node.address_range = nodes[child_index].address_range;
        RemoveNode(child_index, root_index);
        return;
    }

    // Deletion is expected to be exceptionally rare, to the point of
    // being irrelevant in practice since it can only ever happen if an
    // app reads multiple disjoint blocks of a resource and then reads
    // another range covering multiple of those blocks again. Instead
    // of implementing a complex post-delete fixup, mark the root as
    // red and allow the tree to go unbalanced until the next reset.
    if (!node.IsRed() && (node_index != root_index)) {
        nodes[root_index].SetRed(true);
    }

    // We're deleting the a node with one or no children. To avoid
    // special-casing the root node, copy the child node to it and
    // update links as necessary.
    u32 child_index = std::max(l, r);
    u32 parent_index = node.Parent();

    if (child_index) {
        auto& child = nodes[child_index];

        u32 cl = child.Child(0);
        u32 cr = child.Child(1);

        node.SetChild(0, cl);
        node.SetChild(1, cr);

        if (node_index != root_index)
            node.SetRed(child.IsRed());

        node.address_range = child.address_range;

        if (cl) {
            nodes[cl].SetParent(node_index);
        }
        if (cr) {
            nodes[cr].SetParent(node_index);
        }

        child.header = 0;
        FreeNode(child_index);
    } else if (node_index != root_index) {
        // Removing leaf node, update parent link and move on.
        auto& parent = nodes[parent_index];

        const u32 which = parent.Child(1) == node_index;
        parent.SetChild(which, 0);

        node.header = 0;
        FreeNode(node_index);
    } else {
        // Removing root with no children, mark tree as invalid
        const u64 root_bit = 1ULL << (root_index - 1u);
        root_mask_subtree &= ~root_bit;
        root_mask_valid &= ~root_bit;
    }
}

void BarrierTracker::RebalancePostInsert(u32 node_index, u32 root_index) {
    while (node_index != root_index) {
        auto& node = nodes[node_index];
        auto& p = nodes[node.Parent()];

        if (!p.IsRed()) {
            break;
        }

        auto& g = nodes[p.Parent()];

        if (g.Child(1) == node.Parent()) {
            auto& u = nodes[g.Child(0)];

            if (g.Child(0) && u.IsRed()) {
                g.SetRed(true);
                u.SetRed(false);
                p.SetRed(false);
                node_index = p.Parent();
            } else {
                if (p.Child(0) == node_index) {
                    RotateRight(node.Parent(), root_index);
                }
                p.SetRed(false);
                g.SetRed(true);
                RotateLeft(p.Parent(), root_index);
            }
        } else {
            auto& u = nodes[g.Child(1)];
            if (g.Child(1) && u.IsRed()) {
                g.SetRed(true);
                u.SetRed(false);
                p.SetRed(false);
                node_index = p.Parent();
            } else {
                if (p.Child(1) == node_index) {
                    RotateLeft(node.Parent(), root_index);
                }
                p.SetRed(false);
                g.SetRed(true);
                RotateRight(p.Parent(), root_index);
            }
        }
    }
    nodes[root_index].SetRed(false);
}

void BarrierTracker::RotateLeft(u32 node_index, u32 root_index) {
    // This implements rotations in such a way that the node to
    // rotate around does not move. This is important to avoid
    // having a special case for the root node, and avoids having
    // to access the parent or special-case the root node.
    auto& node = nodes[node_index];

    u32 l = node.Child(0);
    u32 r = node.Child(1);
    u32 rl = nodes[r].Child(0);
    u32 rr = nodes[r].Child(1);

    nodes[l].SetParent(r);

    const bool is_red = nodes[r].IsRed();
    nodes[r].SetRed(node.IsRed());
    nodes[r].SetChild(0, l);
    nodes[r].SetChild(1, rl);

    nodes[rr].SetParent(node_index);

    node.SetRed(is_red && node_index != root_index);
    node.SetChild(0, r);
    node.SetChild(1, rr);

    std::swap(node.address_range, nodes[r].address_range);
}

void BarrierTracker::RotateRight(u32 node_index, u32 root_index) {
    auto& node = nodes[node_index];

    u32 l = node.Child(0);
    u32 r = node.Child(1);
    u32 ll = nodes[l].Child(0);
    u32 lr = nodes[l].Child(1);

    nodes[r].SetParent(l);

    const bool is_red = nodes[l].IsRed();
    nodes[l].SetRed(node.IsRed());
    nodes[l].SetChild(0, lr);
    nodes[l].SetChild(1, r);

    nodes[ll].SetParent(node_index);

    node.SetRed(is_red && node_index != root_index);
    node.SetChild(0, ll);
    node.SetChild(1, l);

    std::swap(node.address_range, nodes[l].address_range);
}

} // namespace Vulkan
