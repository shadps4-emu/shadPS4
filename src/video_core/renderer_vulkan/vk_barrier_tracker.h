// SPDX-FileCopyrightText: Copyright DXVK Project
// SPDX-License-Identifier: Zlib

#pragma once

#include <vector>
#include "common/types.h"

namespace Vulkan {

enum class Access : u32 {
    None = 0,
    Read = 1,
    Write = 2,
};

struct AddressRange {
    u64 resource{};
    u64 range_start{};
    u64 range_end{};

    bool Contains(const AddressRange& other) const {
        return resource == other.resource && range_start <= other.range_start &&
               range_end >= other.range_end;
    }

    bool Overlaps(const AddressRange& other) const {
        return resource == other.resource && range_end >= other.range_start &&
               range_start <= other.range_end;
    }

    bool operator<(const AddressRange& other) const {
        return (resource < other.resource) ||
               (resource == other.resource && range_start < other.range_start);
    }
};

/**
 * Node of a red-black tree, consisting of a packed node header as well as a resource address range.
 * GCC generates weird code with bitfields here, so pack manually.
 */
struct BarrierTreeNode {
    constexpr static u64 NodeIndexMask = (1u << 21) - 1u;

    // Packed header with node indices and the node color.
    // [0:0]: Set if the node is red, clear otherwise.
    // [21:1]: Index of the left child node, may be 0.
    // [42:22]: Index of the right child node, may be 0.
    // [43:63]: Index of the parent node, may be 0 for the root.
    u64 header{};

    // Address range of the node
    AddressRange address_range{};

    void SetRed(bool red) {
        header &= ~1ULL;
        header |= u64(red);
    }

    bool IsRed() const {
        return header & 1u;
    }

    void SetParent(u32 node) {
        header &= ~(NodeIndexMask << 43);
        header |= u64(node) << 43;
    }

    void SetChild(u32 index, u32 node) {
        const u32 shift = (index ? 22 : 1);
        header &= ~(NodeIndexMask << shift);
        header |= u64(node) << shift;
    }

    u32 Parent() const {
        return (header >> 43) & NodeIndexMask;
    }

    u32 Child(u32 index) const {
        const u32 shift = (index ? 22 : 1);
        return (header >> shift) & NodeIndexMask;
    }

    bool IsRoot() const {
        return Parent() == 0u;
    }
};

/**
 * Provides a two-part hash table for read and written resource ranges, which is
 * backed by binary trees to handle individual address ranges as well as collisions.
 */
class BarrierTracker {
    constexpr static u32 HashTableSize = 32u;

public:
    explicit BarrierTracker();
    ~BarrierTracker() = default;

    /// Checks whether there is a pending access of a given type
    bool FindRange(const AddressRange& range, Access access_type) const;

    /// Inserts address range for a given access type
    void InsertRange(const AddressRange& range, Access access_type);

    /// Clears the entire structure
    void Clear();

    /// Checks whether any resources are dirty
    bool Empty() const {
        return !root_mask_valid;
    }

private:
    u32 AllocateNode();
    void FreeNode(u32 node);
    u32 FindNode(const AddressRange& range, u32 root_index) const;
    u32 InsertNode(const AddressRange& range, u32 root_index);
    void RemoveNode(u32 node_index, u32 root_index);
    void RebalancePostInsert(u32 node_index, u32 root_index);
    void RotateLeft(u32 node_index, u32 root_index);
    void RotateRight(u32 node_index, u32 root_index);

    constexpr static u32 ComputeRootIndex(const AddressRange& range, Access access) {
        u64 hash = range.resource * 93887;
        hash ^= (hash >> 16);
        return 1 + (hash % HashTableSize) + (access == Access::Write ? HashTableSize : 0);
    }

private:
    u64 root_mask_valid{};
    u64 root_mask_subtree{};
    std::vector<BarrierTreeNode> nodes;
    std::vector<u32> free;
};

} // namespace Vulkan
