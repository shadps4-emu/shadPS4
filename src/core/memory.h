// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <string_view>
#include <vector>
#include <boost/icl/split_interval_map.hpp>
#include "common/enum.h"
#include "common/singleton.h"
#include "common/types.h"
#include "core/address_space.h"
#include "video_core/renderer_vulkan/vk_common.h"

namespace Vulkan {
class Instance;
}

namespace Libraries::Kernel {
struct OrbisQueryInfo;
}

namespace Core {

enum class MemoryProt : u32 {
    NoAccess = 0,
    CpuRead = 1,
    CpuReadWrite = 2,
    GpuRead = 16,
    GpuWrite = 32,
    GpuReadWrite = 38,
};

enum class MemoryMapFlags : u32 {
    NoFlags = 0,
    Fixed = 0x10,
    NoOverwrite = 0x0080,
    NoCoalesce = 0x400000,
};
DECLARE_ENUM_FLAG_OPERATORS(MemoryMapFlags)

enum class VMAType : u32 {
    Free = 0,
    Reserved = 1,
    Direct = 2,
    Flexible = 3,
    Pooled = 4,
    Stack = 5,
};

struct DirectMemoryArea {
    PAddr base = 0;
    size_t size = 0;
    int memory_type = 0;
};

struct VirtualMemoryArea {
    VAddr base = 0;
    size_t size = 0;
    PAddr phys_base = 0;
    VMAType type = VMAType::Free;
    MemoryProt prot = MemoryProt::NoAccess;
    bool disallow_merge = false;
    std::string name = "";

    bool CanMergeWith(const VirtualMemoryArea& next) const {
        if (disallow_merge || next.disallow_merge) {
            return false;
        }
        if (base + size != next.base) {
            return false;
        }
        if (type == VMAType::Direct && phys_base + size != next.phys_base) {
            return false;
        }
        if (prot != next.prot || type != next.type) {
            return false;
        }
        return true;
    }
};

constexpr VAddr SYSTEM_RESERVED = 0x800000000ULL;
constexpr VAddr CODE_BASE_OFFSET = 0x100000000ULL;
constexpr VAddr SYSTEM_MANAGED_MIN = 0x0000040000ULL;
constexpr VAddr SYSTEM_MANAGED_MAX = 0x07FFFFBFFFULL;
constexpr VAddr USER_MIN = 0x1000000000ULL;
constexpr VAddr USER_MAX = 0xFBFFFFFFFFULL;

class MemoryManager {
    using VMAMap = std::map<VAddr, VirtualMemoryArea>;
    using VMAHandle = VMAMap::iterator;

public:
    explicit MemoryManager();
    ~MemoryManager();

    void SetInstance(const Vulkan::Instance* instance_) {
        instance = instance_;
    }

    PAddr Allocate(PAddr search_start, PAddr search_end, size_t size, u64 alignment,
                   int memory_type);

    void Free(PAddr phys_addr, size_t size);

    int MapMemory(void** out_addr, VAddr virtual_addr, size_t size, MemoryProt prot,
                  MemoryMapFlags flags, VMAType type, std::string_view name = "",
                  PAddr phys_addr = -1, u64 alignment = 0);

    void UnmapMemory(VAddr virtual_addr, size_t size);

    int QueryProtection(VAddr addr, void** start, void** end, u32* prot);

    int DirectMemoryQuery(PAddr addr, bool find_next, Libraries::Kernel::OrbisQueryInfo* out_info);

    std::pair<vk::Buffer, size_t> GetVulkanBuffer(VAddr addr);

private:
    VMAHandle FindVMA(VAddr target) {
        // Return first the VMA with base >= target.
        const auto it = vma_map.lower_bound(target);
        if (it != vma_map.end() && it->first == target) {
            return it;
        }
        return std::prev(it);
    }

    VirtualMemoryArea& AddMapping(VAddr virtual_addr, size_t size);

    VMAHandle Split(VMAHandle vma_handle, size_t offset_in_vma);

    VMAHandle MergeAdjacent(VMAHandle iter);

    void MapVulkanMemory(VAddr addr, size_t size);

    void UnmapVulkanMemory(VAddr addr, size_t size);

private:
    AddressSpace impl;
    std::vector<DirectMemoryArea> allocations;
    VMAMap vma_map;

    struct MappedMemory {
        vk::UniqueBuffer buffer;
        vk::UniqueDeviceMemory backing;
        size_t buffer_size;
    };
    std::map<VAddr, MappedMemory> mapped_memories;
    const Vulkan::Instance* instance{};
};

using Memory = Common::Singleton<MemoryManager>;

} // namespace Core
